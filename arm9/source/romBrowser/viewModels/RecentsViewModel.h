#pragma once
#include <memory>
#include <algorithm>
#include <string.h>
#include "../IRomBrowserController.h"
#include "services/gamedata/IGameDataService.h"

/// @brief Cap on the recents list: "recent" implies a short list, and the
///        20 most recent cover it. Favorites are not capped — the whole point
///        is to see every favorite across folders.
#define RECENTS_MAX_ITEMS   20

/// @brief What a game-list bottom sheet shows.
enum class GameListKind
{
    /// @brief Recently played games, most recent first.
    Recents,
    /// @brief Favorite games from every folder, alphabetical.
    Favorites
};

/// @brief View model for the recents/favorites bottom sheets. Snapshots the
///        matching entries at construction time. Entries are copied BY
///        VALUE: the service's array can be reallocated by a favorite toggle
///        while the sheet is still alive (e.g. during its closing
///        animation), which would dangle any pointers into it.
class RecentsViewModel
{
public:
    RecentsViewModel(IRomBrowserController* romBrowserController, GameListKind kind)
        : _romBrowserController(romBrowserController), _kind(kind)
    {
        const auto* gameDataService = romBrowserController->GetGameDataService();
        u32 entryCount = gameDataService->GetEntryCount();
        auto indices = std::make_unique<u32[]>(entryCount);
        u32 matchCount = 0;
        for (u32 i = 0; i < entryCount; i++)
        {
            const auto& entry = gameDataService->GetEntryByIndex(i);
            // both lists navigate on activation, so a stored path is required;
            // favorites/recents from before path tracking self-heal on the
            // next launch or toggle
            if (entry.path.GetString()[0] == 0)
                continue;
            bool matches = kind == GameListKind::Recents
                ? entry.lastPlayed.GetString()[0] != 0
                : entry.favorite;
            if (matches)
                indices[matchCount++] = i;
        }
        std::sort(indices.get(), indices.get() + matchCount,
            [gameDataService, kind] (u32 a, u32 b)
        {
            const auto& entryA = gameDataService->GetEntryByIndex(a);
            const auto& entryB = gameDataService->GetEntryByIndex(b);
            if (kind == GameListKind::Recents)
            {
                // "YYYY-MM-DD HH:MM" sorts chronologically as a plain string
                return strcmp(entryA.lastPlayed.GetString(),
                    entryB.lastPlayed.GetString()) > 0;
            }
            return strcasecmp(entryA.fileName.GetString(),
                entryB.fileName.GetString()) < 0;
        });
        // recents is capped (a short "recent" list); favorites shows all
        _itemCount = kind == GameListKind::Recents
            ? std::min<u32>(matchCount, RECENTS_MAX_ITEMS)
            : matchCount;
        _items = std::make_unique<GameDataEntry[]>(_itemCount);
        for (u32 i = 0; i < _itemCount; i++)
            _items[i] = gameDataService->GetEntryByIndex(indices[i]);
        // Deliberately no "does the file still exist" pass here. Checking every
        // row means SD I/O on the main thread inside the frame loop, and one
        // f_stat is a full directory scan, so a library with many favorites
        // stalls visibly - the same failure the empty-folder probe once caused.
        // Rows whose file is gone stay listed and are handled in ActivateItem.
    }

    constexpr GameListKind GetKind() const { return _kind; }

    u32 GetItemCount() const { return _itemCount; }
    const GameDataEntry& GetItem(u32 index) const { return _items[index]; }

    constexpr int GetSelectedItem() const { return _selectedItem; }
    void SetSelectedItem(int selectedItem) { _selectedItem = selectedItem; }

    void ActivateItem(int index)
    {
        if (index >= 0 && (u32)index < _itemCount)
        {
            // The file may be gone (renamed or deleted outside the launcher) or
            // the stored path stale after a reorganisation. Navigating to a
            // missing file falls back to the card root, which reads as the
            // launcher randomly jumping somewhere, so ignore the press instead.
            // One check, and only on a deliberate activation - not a scan of
            // every row each time the panel opens.
            FILINFO fileInfo;
            if (f_stat(_items[index].path.GetString(), &fileInfo) != FR_OK)
            {
                LOG_ERROR("Game data: %s is not at %s any more\n",
                    _items[index].fileName.GetString(), _items[index].path.GetString());
                return;
            }
            // navigates to the game's folder and preselects it, the same
            // mechanism used for lastUsedFilePath at startup
            _romBrowserController->NavigateToPath(_items[index].path.GetString());
        }
    }

    void Close()
    {
        if (_kind == GameListKind::Recents)
            _romBrowserController->HideRecents();
        else
            _romBrowserController->HideFavorites();
    }

private:
    IRomBrowserController* _romBrowserController;
    GameListKind _kind;
    std::unique_ptr<GameDataEntry[]> _items;
    u32 _itemCount = 0;
    int _selectedItem = -1;
};
