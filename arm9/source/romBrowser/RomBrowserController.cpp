#include "common.h"
#include <array>
#include "picoLoaderBootstrap.h"
#include "PicoLoaderProcess.h"
#include "settings/SettingsProcess.h"
#include "FileType/ExtensionFileTypeProvider.h"
#include "FileType/FileType.h"
#include "SdFolderFactory.h"
#include "services/settings/IAppSettingsService.h"
#include "cheats/UsrCheatRepositoryFactory.h"
#include "cheats/EmptyCheatRepository.h"
#include "cheats/PicoLoaderCheatDataFactory.h"
#include "services/gamedata/IGameDataService.h"
#include "bgm/IBgmService.h"
#include "core/mini-printf.h"
#include "rtcIpc.h"
#include "RomBrowserController.h"

RomBrowserController::RomBrowserController(
    IAppSettingsService* appSettingsService, IGameDataService* gameDataService,
    IBgmService* bgmService, TaskQueueBase* ioTaskQueue, TaskQueueBase* bgTaskQueue)
    : _appSettingsService(appSettingsService)
    , _gameDataService(gameDataService)
    , _bgmService(bgmService)
    , _ioTaskQueue(ioTaskQueue), _bgTaskQueue(bgTaskQueue)
    , _fileTypeProvider(appSettingsService->GetAppSettings()) { }

void RomBrowserController::NavigateToPath(const TCHAR* name)
{
    StringUtil::Copy(_navigatePath, name, sizeof(_navigatePath) / sizeof(_navigatePath[0]));
    _stateMachine.Fire(RomBrowserStateTrigger::Navigate);
}

void RomBrowserController::LaunchFile(const FileInfo& fileInfo, const char* gameCode)
{
    _triggerFileInfo = FileInfo(fileInfo);
    StringUtil::Copy(_triggerGameCode, gameCode ? gameCode : "",
        sizeof(_triggerGameCode) / sizeof(_triggerGameCode[0]));
    _stateMachine.Fire(RomBrowserStateTrigger::Launch);
}

void RomBrowserController::LaunchRandomGame()
{
    if (!_romBrowserViewModel.IsValid())
        return;
    auto& fileInfoManager = _romBrowserViewModel->GetFileInfoManager();
    u32 gameCount = fileInfoManager.GetGameCount();
    if (gameCount == 0)
        return;
    u32 pick = gRandomGenerator->NextU32(gameCount);
    for (u32 i = 0; i < fileInfoManager.GetItemCount(); i++)
    {
        const auto& item = fileInfoManager.GetItem(i);
        if (item.GetFileType()->GetClassification() != FileTypeClassification::Game)
            continue;
        if (pick == 0)
        {
            // an off-screen random pick usually has no file info loaded yet;
            // the launch then records by name only, which self-heals later
            const char* gameCode = nullptr;
            if (fileInfoManager.IsFileInfoLoaded(i))
            {
                const auto* info = fileInfoManager.GetInternalFileInfo(i);
                if (info)
                    gameCode = info->GetGameCode();
            }
            LaunchFile(item, gameCode);
            return;
        }
        pick--;
    }
}

void RomBrowserController::ToggleFavorite(const FileInfo& fileInfo, const char* gameCode)
{
    _gameDataService->ToggleFavorite(fileInfo.GetFileName(), gameCode);
    _gameDataService->SaveAsync(_ioTaskQueue);
    if (_favoritesFilter)
    {
        // an unfavorited game must drop out of the filtered view
        _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
    }
}

void RomBrowserController::ToggleFavoritesFilter()
{
    _favoritesFilter = !_favoritesFilter;
    _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
}

void RomBrowserController::ShowGameInfo(const FileInfo& fileInfo)
{
    _triggerFileInfo = FileInfo(fileInfo);
    _stateMachine.Fire(RomBrowserStateTrigger::ShowGameInfo);
}

void RomBrowserController::HideGameInfo()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideGameInfo);
}

void RomBrowserController::ShowDisplaySettings()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowDisplaySettings);
}

void RomBrowserController::ShowRecents()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowRecents);
}

void RomBrowserController::HideRecents()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideRecents);
}

void RomBrowserController::ShowStatistics()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowStatistics);
}

void RomBrowserController::RequestDeleteSelected()
{
    if (!_romBrowserViewModel.IsValid())
        return;
    int selectedItem = _romBrowserViewModel->GetSelectedItem();
    if (selectedItem < 0)
        return;
    const auto& item = _romBrowserViewModel->GetFileInfoManager().GetItem(selectedItem);
    // only games: deleting folders would need recursion, and deleting random
    // support files from the launcher is asking for trouble
    if (item.GetFileType()->GetClassification() != FileTypeClassification::Game)
        return;

    StringUtil::Copy(_deleteRomFileName, item.GetFileName(),
        sizeof(_deleteRomFileName) / sizeof(_deleteRomFileName[0]));
    const char* gameCode = nullptr;
    auto& fileInfoManager = _romBrowserViewModel->GetFileInfoManager();
    if (fileInfoManager.IsFileInfoLoaded(selectedItem))
    {
        const auto* info = fileInfoManager.GetInternalFileInfo(selectedItem);
        if (info)
            gameCode = info->GetGameCode();
    }
    StringUtil::Copy(_deleteGameCode, gameCode ? gameCode : "",
        sizeof(_deleteGameCode) / sizeof(_deleteGameCode[0]));
    // "<name minus extension>.sav" is the save convention used by the loader
    // and the emulators next to their roms
    StringUtil::Copy(_deleteSaveFileName, _deleteRomFileName,
        sizeof(_deleteSaveFileName) / sizeof(_deleteSaveFileName[0]));
    TCHAR* dot = strrchr(_deleteSaveFileName, '.');
    if (dot)
        *dot = 0;
    strlcat(_deleteSaveFileName, ".sav", sizeof(_deleteSaveFileName));
    FILINFO fileInfo;
    _deleteHasSave = f_stat(_deleteSaveFileName, &fileInfo) == FR_OK;

    _stateMachine.Fire(RomBrowserStateTrigger::ShowDeleteConfirm);
}

void RomBrowserController::CancelDelete()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideDeleteConfirm);
}

void RomBrowserController::ConfirmDelete()
{
    _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        FRESULT result = f_unlink(_deleteRomFileName);
        if (result != FR_OK)
            LOG_ERROR("Couldn't delete file (%d)\n", result);
        else if (_deleteHasSave)
            f_unlink(_deleteSaveFileName);
        _deleteCompleted = true;
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::HideStatistics()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideStatistics);
}

void RomBrowserController::HideDisplaySettings()
{
    if (_saveSettingsPending)
    {
        _saveSettingsPending = false;
        _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
        {
            _appSettingsService->Save();
            return TaskResult<void>::Completed();
        });
    }
    _stateMachine.Fire(RomBrowserStateTrigger::HideDisplaySettings);
}

void RomBrowserController::GotoSettingsScreen()
{
    _stateMachine.Fire(RomBrowserStateTrigger::GotoSettingsScreen);
}

void RomBrowserController::SetRomBrowserDisplaySettings(
    const RomBrowserDisplaySettings& romBrowserDisplaySettings)
{
    _appSettingsService->GetAppSettings().romBrowserDisplaySettings = romBrowserDisplaySettings;
    _saveSettingsPending = true;
    _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
}

void RomBrowserController::Update()
{
    if (_deleteCompleted)
    {
        _deleteCompleted = false;
        // the deleted game's favorite/stats entry goes with it
        _gameDataService->RemoveEntry(_deleteRomFileName,
            _deleteGameCode[0] != 0 ? _deleteGameCode : nullptr);
        _gameDataService->SaveAsync(_ioTaskQueue);
        // reload the current folder so the deleted file disappears
        NavigateToPath(".");
    }
    _stateMachine.Update();
    if (_stateMachine.HasStateChanged())
    {
        HandleTrigger();
    }
    switch (_stateMachine.GetCurrentState())
    {
        case RomBrowserState::Start:
        {
            LOG_DEBUG("RomBrowserState::Start\n");
            const auto& lastUsed = _appSettingsService->GetAppSettings().lastUsedFilePath;
            if (strlen(lastUsed.GetString()) != 0)
            {
                NavigateToPath(lastUsed.GetString());
            }
            else
            {
                NavigateToPath("/");
            }
            break;
        }
        case RomBrowserState::LoadingFolder:
        {
            if (_navigateTask.GetTask().IsCompletedSuccessfully())
            {
                _navigateTask.Dispose();
                _stateMachine.Fire(RomBrowserStateTrigger::FolderLoadDone);
            }
            break;
        }
        case RomBrowserState::Launching:
        default:
        {
            break;
        }
    }
}

void RomBrowserController::HandleTrigger()
{
    switch (_stateMachine.GetLastTrigger())
    {
        case RomBrowserStateTrigger::Navigate:
            HandleNavigateTrigger();
            break;

        case RomBrowserStateTrigger::FolderLoadDone:
            HandleFolderLoadDoneTrigger();
            break;

        case RomBrowserStateTrigger::Launch:
            HandleLaunchTrigger();
            break;

        case RomBrowserStateTrigger::ChangeDisplayMode:
            HandleChangeDisplayModeTrigger();
            break;

        case RomBrowserStateTrigger::GotoSettingsScreen:
            HandleGotoSettingsScreenTrigger();
            break;

        default:
            break;
    }
}

void RomBrowserController::HandleNavigateTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::Navigate\n");
    _navigateTask = _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        if (!_coverRepository)
        {
            _coverRepository = std::make_unique<CoverRepository>();
            _coverRepository->Initialize();
        }
        if (!_iconRepository)
        {
            _iconRepository = std::make_unique<IconRepository>();
            _iconRepository->Initialize();
        }
        if (!_bannerRepository)
        {
            _bannerRepository = std::make_unique<BannerRepository>();
            _bannerRepository->Initialize();
        }
        if (!_cheatRepository)
        {
            _cheatRepository = UsrCheatRepositoryFactory().FromUsrCheatDat("/_pico/usrcheat.dat");
            if (!_cheatRepository)
            {
                // When usrcheat.dat is not found or cannot be read use a dummy empty cheat repository
                _cheatRepository = std::make_unique<EmptyCheatRepository>();
            }
        }

        u64 startTick = gTickCounter.GetValue();
        _navigateFileName = nullptr;
        if (strcmp(_navigatePath, "/") != 0) // can't f_stat on root dir
        {
            FILINFO fileInfo;
            if (f_stat(_navigatePath, &fileInfo) != FR_OK)
            {
                StringUtil::Copy(_navigatePath, "/", sizeof(_navigatePath) / sizeof(_navigatePath[0]));
            }
            else if (!(fileInfo.fattrib & AM_DIR))
            {
                _navigateFileName = strrchr(_navigatePath, '/') + 1;
                _navigateFileName[-1] = 0;
            }
        }
        f_chdir(_navigatePath);
        SdFolderFactory sdFolderFactory { &_fileTypeProvider };
        _newSdFolder = sdFolderFactory.CreateFromPath(".");
        u64 endTick = gTickCounter.GetValue();
        LOG_DEBUG("Loading files in folder took: %d us\n", (u32)TickCounter::TicksToMicroSeconds(endTick - startTick));

        // folders can bring their own music via a bgm.bcstm inside them
        FILINFO bgmFileInfo;
        if (f_stat("bgm.bcstm", &bgmFileInfo) == FR_OK && !(bgmFileInfo.fattrib & AM_DIR))
        {
            TCHAR bgmPath[256];
            f_getcwd(bgmPath, sizeof(bgmPath) / sizeof(bgmPath[0]));
            int idx = strlcat(bgmPath, "/", sizeof(bgmPath));
            if (bgmPath[idx - 2] == '/')
            {
                bgmPath[idx - 1] = 0;
            }
            strlcat(bgmPath, "bgm.bcstm", sizeof(bgmPath));
            _bgmService->UpdateBgmForFolder(bgmPath);
        }
        else
        {
            _bgmService->UpdateBgmForFolder(nullptr);
        }
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::HandleFolderLoadDoneTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::FolderLoadDone\n");
    _romBrowserViewModel.Reset();
    _sdFolder = std::move(_newSdFolder);
    _romBrowserViewModel = SharedPtr<RomBrowserViewModel>::MakeShared(this, _navigateFileName);
}

void RomBrowserController::HandleLaunchTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::Launch\n");
    rtc_datetime_t dateTime;
    rtc_readDateTime(&dateTime);
    char lastPlayed[20];
    // the rtc registers hold BCD values, which %x renders as decimal digits
    mini_snprintf(lastPlayed, sizeof(lastPlayed), "20%02x-%02x-%02x %02x:%02x",
        dateTime.date.year, dateTime.date.month, dateTime.date.monthDay,
        dateTime.time.hour, dateTime.time.minute);
    // same full-path construction as UpdateLastUsedFilepath, but into a local
    // buffer: _navigatePath belongs to the navigation flow
    TCHAR fullPath[256];
    f_getcwd(fullPath, sizeof(fullPath) / sizeof(fullPath[0]));
    int idx = strlcat(fullPath, "/", sizeof(fullPath));
    if (fullPath[idx - 2] == '/')
    {
        fullPath[idx - 1] = 0;
    }
    strlcat(fullPath, _triggerFileInfo.GetFileName(), sizeof(fullPath));
    _gameDataService->RecordLaunch(_triggerFileInfo.GetFileName(),
        _triggerGameCode[0] != 0 ? _triggerGameCode : nullptr, fullPath, lastPlayed);
    _gameDataService->SaveAsync(_ioTaskQueue);
    _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        UpdateLastUsedFilepath();
        SetPicoLoaderParams();
        LoadCheats();
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::HandleChangeDisplayModeTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::ChangeDisplayMode\n");
    _romBrowserViewModel = SharedPtr<RomBrowserViewModel>::MakeShared(this);
}

void RomBrowserController::HandleGotoSettingsScreenTrigger()
{
    gProcessManager.Goto<SettingsProcess>();
}

void RomBrowserController::UpdateLastUsedFilepath()
{
    f_getcwd(_navigatePath, sizeof(_navigatePath) / sizeof(_navigatePath[0]));
    int idx = strlcat(_navigatePath, "/", sizeof(_navigatePath));
    if (_navigatePath[idx - 2] == '/')
    {
        _navigatePath[idx - 1] = 0;
    }
    strlcat(_navigatePath, _triggerFileInfo.GetFileName(), sizeof(_navigatePath));
    _appSettingsService->GetAppSettings().lastUsedFilePath = _navigatePath;
    _appSettingsService->Save();
}

void RomBrowserController::SetPicoLoaderParams() const
{
    auto loadParams = pload_getLoadParams();
    loadParams->savePath[0] = 0;
    loadParams->arguments[0] = 0;
    loadParams->argumentsLength = 0;
    if (_triggerFileInfo.GetFileType()->TrySetLaunchParameters(loadParams, _navigatePath))
    {
        gProcessManager.Goto<PicoLoaderProcess>();
    }
    else
    {
        LOG_FATAL("Failed to set launch parameters.\n");
    }
}

void RomBrowserController::LoadCheats() const
{
    auto cheats = _cheatRepository->GetCheatsForGame(_triggerFileInfo.GetFastFileRef());
    auto cheatData = PicoLoaderCheatDataFactory().CreateCheatData(cheats);
    pload_setCheatData(cheatData);
}
