#include "common.h"
#include <string.h>
#include "core/mini-printf.h"
#include "core/StringUtil.h"
#include "Pcm16FileAudioStream.h"
#include "BcstmAudioStream.h"
#include "romBrowser/SdFolder.h"
#include "romBrowser/SdFolderFactory.h"
#include "romBrowser/FileType/NullFileTypeProvider.h"
#include "BgmService.h"

bool BgmService::StartBgm(const TCHAR* filePath)
{
    auto stream = std::make_unique<BcstmAudioStream>();
    if (!stream->Open(filePath))
        return false;

    return _audioStreamPlayer->StartPlayback(std::move(stream));
}

void BgmService::StartBgmFromConfig()
{
    // config playback invalidates the folder-bgm dedup state; without this a
    // fresh App (settings round-trip) would refuse to restart a folder's bgm
    _playingFolderBgm = false;
    _currentFolderBgmPath[0] = 0;
    TCHAR pathBuffer[128];
    mini_snprintf(pathBuffer, sizeof(pathBuffer), "/_pico/themes/%s/bgm", _appSettingsService.GetAppSettings().theme.GetString());
    NullFileTypeProvider fileTypeProvider;
    auto bgmFolder = SdFolderFactory(&fileTypeProvider).CreateFromPath(pathBuffer);
    if (!bgmFolder || bgmFolder->GetFileCount() == 0)
    {
        StopBgm();
        return;
    }
    u32 bgmToPlay = _randomGenerator.NextU32(bgmFolder->GetFileCount());
    auto stream = std::make_unique<BcstmAudioStream>();
    if (!stream->Open(bgmFolder->GetFiles()[bgmToPlay]->GetFastFileRef()))
    {
        StopBgm();
        return;
    }

    _audioStreamPlayer->StartPlayback(std::move(stream));
}

void BgmService::UpdateBgmForFolder(const TCHAR* folderBgmPath)
{
    if (folderBgmPath)
    {
        if (_playingFolderBgm && !strcasecmp(_currentFolderBgmPath, folderBgmPath))
            return;
        if (StartBgm(folderBgmPath))
        {
            _playingFolderBgm = true;
            StringUtil::Copy(_currentFolderBgmPath, folderBgmPath,
                sizeof(_currentFolderBgmPath) / sizeof(_currentFolderBgmPath[0]));
            return;
        }
        // unreadable folder bgm: fall back to the theme music below
    }
    if (_playingFolderBgm)
    {
        _playingFolderBgm = false;
        _currentFolderBgmPath[0] = 0;
        StartBgmFromConfig();
    }
}

void BgmService::StopBgm()
{
    _audioStreamPlayer->StopPlayback();
}
