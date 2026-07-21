#pragma once
#include "core/SharedPtr.h"
#include "services/settings/AppSettings.h"

class SdFolder;
class RomBrowserStateMachine;
class RomBrowserViewModel;
class FileInfo;
class TaskQueueBase;
class ICoverRepository;
class IIconRepository;
class IBannerRepository;
class ICheatRepository;
class IGameDataService;
class IBgmService;

class IRomBrowserController
{
public:
    virtual ~IRomBrowserController() = 0;

    virtual void NavigateUp() = 0;
    virtual void NavigateToPath(const TCHAR* name) = 0;
    /// @brief gameCode is the file's internal game code when the caller has
    ///        it loaded, or nullptr; it keys the game's persisted data.
    virtual void LaunchFile(const FileInfo& fileInfo, const char* gameCode = nullptr) = 0;
    virtual void LaunchRandomGame() = 0;
    virtual void ToggleFavorite(const FileInfo& fileInfo, const char* gameCode = nullptr) = 0;
    virtual void ToggleCompleted(const FileInfo& fileInfo, const char* gameCode = nullptr) = 0;
    virtual void ToggleFavoritesFilter() = 0;
    virtual bool IsFavoritesFilterEnabled() const = 0;
    virtual void ToggleCompletedFilter() = 0;
    virtual bool IsCompletedFilterEnabled() const = 0;
    virtual void ShowGameInfo(const FileInfo& fileInfo) = 0;
    virtual void HideGameInfo() = 0;
    virtual void ShowDisplaySettings() = 0;
    virtual void HideDisplaySettings() = 0;
    virtual void ShowRecents() = 0;
    virtual void HideRecents() = 0;
    virtual void ShowStatistics() = 0;
    virtual void HideStatistics() = 0;
    virtual void RequestDeleteSelected() = 0;
    virtual void CancelDelete() = 0;
    virtual void ConfirmDelete() = 0;
    /// @brief File that will be deleted. Deliberately a dedicated buffer, NOT
    ///        _triggerFileInfo: the launch flows overwrite that one, and a
    ///        delete must never act on anything but the file the user saw.
    virtual const char* GetDeleteRomFileName() const = 0;
    /// @brief Save file that will be deleted along with the game, empty when
    ///        there is none. Valid while the delete confirmation is shown.
    virtual const char* GetDeleteSaveFileName() const = 0;
    virtual void GotoSettingsScreen() = 0;

    virtual void Update() = 0;

    virtual const SdFolder& GetSdFolder() const = 0;

    virtual const RomBrowserStateMachine& GetStateMachine() const = 0;

    virtual const SharedPtr<RomBrowserViewModel>& GetRomBrowserViewModel() = 0;

    virtual TaskQueueBase* GetIoTaskQueue() const = 0;
    virtual TaskQueueBase* GetBgTaskQueue() const = 0;
    virtual const ICoverRepository& GetCoverRepository() const = 0;
    virtual const IIconRepository& GetIconRepository() const = 0;
    virtual const IBannerRepository& GetBannerRepository() const = 0;
    virtual const ICheatRepository& GetCheatRepository() const = 0;
    virtual IGameDataService* GetGameDataService() = 0;

    virtual const RomBrowserDisplaySettings& GetRomBrowserDisplaySettings() const = 0;

    virtual void SetRomBrowserDisplaySettings(
        const RomBrowserDisplaySettings& romBrowserDisplaySettings) = 0;

    /// @brief DS Lite backlight level (0..3), or -1 when the user never
    ///        picked one (the firmware's level is left untouched then).
    virtual int GetBacklightLevel() const = 0;
    virtual void SetBacklightLevel(int level) = 0;

    virtual const FileInfo& GetTriggerFileInfo() const = 0;
};

inline IRomBrowserController::~IRomBrowserController() { }
