#pragma once
#include "core/String.h"

/// @brief Per-game persisted data, keyed by file name (same convention as
///        the /_pico/icons|covers/user folders).
struct GameDataEntry
{
    String<char, 96> fileName;
    /// @brief Internal game code (NDS/GBA header), empty when the file has
    ///        none. Lookups prefer it: it survives renames and moves.
    String<char, 8> gameCode;
    u32 launchCount = 0;
    /// @brief Accumulated play time. A session spans from launching the game
    ///        until the next launcher boot, so it is an approximation.
    u32 playMinutes = 0;
    bool favorite = false;
    /// @brief "YYYY-MM-DD HH:MM", empty when never launched. Lexicographic
    ///        order equals chronological order.
    String<char, 20> lastPlayed;
    /// @brief Full path of the file at its last launch, empty when never
    ///        launched. Used by the recents list to navigate back to it.
    String<char, 256> path;
};
