#pragma once
#include <string.h>
#include "../IRomBrowserController.h"
#include "services/gamedata/IGameDataService.h"

#define STATISTICS_TOP_COUNT   3

/// @brief View model for the statistics bottom sheet. Computes everything at
///        construction time, copying values instead of keeping pointers into
///        the game data service (its array can be reallocated while the sheet
///        is still alive).
class StatisticsViewModel
{
public:
    explicit StatisticsViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController)
    {
        const auto* gameDataService = romBrowserController->GetGameDataService();
        u32 entryCount = gameDataService->GetEntryCount();
        const GameDataEntry* top[STATISTICS_TOP_COUNT] = {};
        const GameDataEntry* last = nullptr;
        for (u32 i = 0; i < entryCount; i++)
        {
            const auto& entry = gameDataService->GetEntryByIndex(i);
            if (entry.favorite)
                _favoriteCount++;
            if (entry.launchCount == 0)
                continue;
            _playedCount++;
            _totalLaunches += entry.launchCount;
            for (u32 t = 0; t < STATISTICS_TOP_COUNT; t++)
            {
                if (!top[t] || entry.launchCount > top[t]->launchCount)
                {
                    for (u32 m = STATISTICS_TOP_COUNT - 1; m > t; m--)
                        top[m] = top[m - 1];
                    top[t] = &entry;
                    break;
                }
            }
            if (!last || strcmp(entry.lastPlayed.GetString(), last->lastPlayed.GetString()) > 0)
                last = &entry;
        }
        for (u32 t = 0; t < STATISTICS_TOP_COUNT; t++)
        {
            if (top[t])
                _topEntries[_topCount++] = *top[t];
        }
        if (last)
            _lastPlayed = *last;
    }

    u32 GetPlayedCount() const { return _playedCount; }
    u32 GetFavoriteCount() const { return _favoriteCount; }
    u32 GetTotalLaunches() const { return _totalLaunches; }
    u32 GetTopCount() const { return _topCount; }
    const GameDataEntry& GetTopEntry(u32 index) const { return _topEntries[index]; }
    const GameDataEntry& GetLastPlayed() const { return _lastPlayed; }

    void Close()
    {
        _romBrowserController->HideStatistics();
    }

private:
    IRomBrowserController* _romBrowserController;
    u32 _playedCount = 0;
    u32 _favoriteCount = 0;
    u32 _totalLaunches = 0;
    u32 _topCount = 0;
    GameDataEntry _topEntries[STATISTICS_TOP_COUNT];
    GameDataEntry _lastPlayed;
};
