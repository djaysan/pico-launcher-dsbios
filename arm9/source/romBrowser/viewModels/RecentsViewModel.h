#pragma once
#include <memory>
#include <algorithm>
#include <string.h>
#include "../IRomBrowserController.h"
#include "services/gamedata/IGameDataService.h"

#define RECENTS_MAX_ITEMS   20

/// @brief View model for the recents bottom sheet. Snapshots the recently
///        played entries (sorted most recent first) at construction time.
///        Entries are copied BY VALUE: the service's array can be reallocated
///        by a favorite toggle while the sheet is still alive (e.g. during
///        its closing animation), which would dangle any pointers into it.
class RecentsViewModel
{
public:
    explicit RecentsViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController)
    {
        const auto* gameDataService = romBrowserController->GetGameDataService();
        u32 entryCount = gameDataService->GetEntryCount();
        auto indices = std::make_unique<u32[]>(entryCount);
        u32 playedCount = 0;
        for (u32 i = 0; i < entryCount; i++)
        {
            const auto& entry = gameDataService->GetEntryByIndex(i);
            if (entry.lastPlayed.GetString()[0] != 0 && entry.path.GetString()[0] != 0)
                indices[playedCount++] = i;
        }
        // "YYYY-MM-DD HH:MM" sorts chronologically as a plain string
        std::sort(indices.get(), indices.get() + playedCount, [gameDataService] (u32 a, u32 b)
        {
            return strcmp(gameDataService->GetEntryByIndex(a).lastPlayed.GetString(),
                gameDataService->GetEntryByIndex(b).lastPlayed.GetString()) > 0;
        });
        _itemCount = std::min<u32>(playedCount, RECENTS_MAX_ITEMS);
        for (u32 i = 0; i < _itemCount; i++)
            _items[i] = gameDataService->GetEntryByIndex(indices[i]);
    }

    u32 GetItemCount() const { return _itemCount; }
    const GameDataEntry& GetItem(u32 index) const { return _items[index]; }

    constexpr int GetSelectedItem() const { return _selectedItem; }
    void SetSelectedItem(int selectedItem) { _selectedItem = selectedItem; }

    void ActivateItem(int index)
    {
        if (index >= 0 && (u32)index < _itemCount)
        {
            // navigates to the game's folder and preselects it, the same
            // mechanism used for lastUsedFilePath at startup
            _romBrowserController->NavigateToPath(_items[index].path.GetString());
        }
    }

    void Close()
    {
        _romBrowserController->HideRecents();
    }

private:
    IRomBrowserController* _romBrowserController;
    GameDataEntry _items[RECENTS_MAX_ITEMS];
    u32 _itemCount = 0;
    int _selectedItem = -1;
};
