#pragma once
#include "core/String.h"

/// @brief Per-game persisted data, keyed by file name (same convention as
///        the /_pico/icons|covers/user folders).
struct GameDataEntry
{
    String<char, 96> fileName;
    u32 launchCount = 0;
    bool favorite = false;
    /// @brief "YYYY-MM-DD HH:MM", empty when never launched.
    String<char, 20> lastPlayed;
};
