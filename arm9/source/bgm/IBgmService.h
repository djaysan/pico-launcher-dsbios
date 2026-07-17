#pragma once
#include "fat/ff.h"

/// @brief Interface for a background music service.
class IBgmService
{
public:
    virtual ~IBgmService() = 0;

    /// @brief Starts playback of the given file.
    /// @param filePath The file to play.
    /// @return True if playback was successfully started, or false otherwise.
    virtual bool StartBgm(const TCHAR* filePath) = 0;

    /// @brief Starts playback of the background music according to the app config.
    virtual void StartBgmFromConfig() = 0;

    /// @brief Plays the given folder music, or falls back to the theme music
    ///        when null. Tracks that are already playing are not restarted.
    /// @param folderBgmPath Full path of the folder's bgm file, or nullptr.
    virtual void UpdateBgmForFolder(const TCHAR* folderBgmPath) = 0;

    /// @brief If currently playing, stops playback.
    virtual void StopBgm() = 0;
};

inline IBgmService::~IBgmService() { }
