#pragma once
#include "core/String.h"

/// @brief Per-game persisted data, keyed by file name (same convention as
///        the /_pico/icons|covers/user folders).
struct GameDataEntry
{
    String<char, 96> fileName;
    u32 launchCount = 0;
    bool favorite = false;
    /// @brief "YYYY-MM-DD HH:MM", empty when never launched. Lexicographic
    ///        order equals chronological order.
    String<char, 20> lastPlayed;
    /// @brief Full path of the file at its last launch, empty when never
    ///        launched. Used by the recents list to navigate back to it.
    String<char, 256> path;
};
