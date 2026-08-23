#pragma once
#include "core/String.h"

/// @brief Persisted data for ONE rom file, keyed by its file name (same
///        convention as the /_pico/icons|covers/user folders). Marks and play
///        statistics belong to the file, so two copies of a game are tracked
///        separately and renaming a rom starts it over.
struct GameDataEntry
{
    String<char, 96> fileName;
    /// @brief Internal game code (NDS/GBA header), empty when the file has
    ///        none. Stored as information only - it is never used to look an
    ///        entry up, because the browser filter only ever sees file names and
    ///        the two would then disagree (issue #7).
    String<char, 8> gameCode;
    u32 launchCount = 0;
    /// @brief Accumulated play time. A session spans from launching the game
    ///        until the next launcher boot, so it is an approximation.
    u32 playMinutes = 0;
    bool favorite = false;
    /// @brief Marked as finished. No longer written or shown: the check mark
    ///        was dropped and long-press X now asks to delete instead. Kept so
    ///        the json format is unchanged for tools that read it (PicoDex),
    ///        and so existing marks are ignored rather than erased.
    bool completed = false;
    /// @brief "YYYY-MM-DD HH:MM", empty when never launched. Lexicographic
    ///        order equals chronological order.
    String<char, 20> lastPlayed;
    /// @brief Full path of the file at its last launch, empty when never
    ///        launched. Used by the recents list to navigate back to it.
    String<char, 256> path;
};
