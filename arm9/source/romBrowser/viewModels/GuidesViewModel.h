#pragma once
#include <memory>
#include <algorithm>
#include <string.h>
#include "core/String.h"
#include "fat/Directory.h"
#include "../IRomBrowserController.h"

/// @brief Folder holding the guides. No volume id, so it resolves against
///        whatever drive is mounted (fat: on a flashcard, sd: on the dsi) -
///        the same convention the .txt file association uses.
#define GUIDES_FOLDER_PATH  "/guides"

/// @brief View model for the guides bottom sheet: every .txt in /guides,
///        alphabetical, with the guide for the highlighted game preselected.
class GuidesViewModel
{
public:
    /// @brief Longest guide file name that is listed. Longer ones are skipped
    ///        rather than truncated into a name that opens the wrong file.
    static constexpr u32 kMaxNameLength = 128;

    /// @param guidePath Full path of the guide that belongs to the highlighted
    ///        game, or an empty string when it has none. Only used to preselect.
    GuidesViewModel(IRomBrowserController* romBrowserController, const char* guidePath)
        : _romBrowserController(romBrowserController)
    {
        // One directory scan, on the main thread. /guides holds only guides, so
        // this reads a few hundred entries - not the card-wide walk the
        // empty-folder probe does - and the sheet spends longer sliding up than
        // this takes.
        // ponytail: move onto the io task queue if a huge /guides ever stutters.
        Directory directory;
        if (directory.Open(GUIDES_FOLDER_PATH) != FR_OK)
        {
            LOG_ERROR("No %s folder on this card\n", GUIDES_FOLDER_PATH);
            return;
        }
        FILINFO fileInfo;
        while (directory.Read(&fileInfo) == FR_OK && fileInfo.fname[0] != 0)
        {
            if (fileInfo.fattrib & AM_DIR)
                continue;
            const char* dot = strrchr(fileInfo.fname, '.');
            if (!dot || strcasecmp(dot, ".txt") != 0)
                continue;
            if (strlen(fileInfo.fname) > kMaxNameLength)
                continue;
            if (_itemCount == _itemCapacity)
            {
                u32 newCapacity = _itemCapacity == 0 ? 64 : _itemCapacity * 2;
                auto newItems = std::make_unique<GuideName[]>(newCapacity);
                for (u32 i = 0; i < _itemCount; i++)
                    newItems[i] = _items[i];
                _items = std::move(newItems);
                _itemCapacity = newCapacity;
            }
            _items[_itemCount++] = (const char*)fileInfo.fname;
        }
        std::sort(_items.get(), _items.get() + _itemCount,
            [] (const GuideName& a, const GuideName& b)
        {
            return strcasecmp(a.GetString(), b.GetString()) < 0;
        });

        // Preselect the highlighted game's own guide when there is one, so the
        // list opens on it instead of on whatever sorts first.
        const char* wantedName = GuideFileNameFromPath(guidePath);
        if (wantedName)
        {
            for (u32 i = 0; i < _itemCount; i++)
            {
                if (strcasecmp(_items[i].GetString(), wantedName) == 0)
                {
                    _initialSelectedItem = (int)i;
                    break;
                }
            }
        }
    }

    /// @brief File name part of a "/guides/<name>.txt" path, or nullptr when
    ///        the path is empty.
    static const char* GuideFileNameFromPath(const char* guidePath)
    {
        if (!guidePath || guidePath[0] == 0)
            return nullptr;
        const char* slash = strrchr(guidePath, '/');
        return slash ? slash + 1 : guidePath;
    }

    u32 GetItemCount() const { return _itemCount; }
    const char* GetItem(u32 index) const { return _items[index].GetString(); }

    constexpr int GetInitialSelectedItem() const { return _initialSelectedItem; }

    constexpr int GetSelectedItem() const { return _selectedItem; }
    void SetSelectedItem(int selectedItem) { _selectedItem = selectedItem; }

    /// @brief The sheet reads this after an activation and opens the guide.
    ///        The list stays where it is: picking is a bottom screen job and
    ///        reading is a top screen one.
    constexpr int ConsumeActivatedItem()
    {
        int index = _activatedItem;
        _activatedItem = -1;
        return index;
    }

    void ActivateItem(int index)
    {
        if (index >= 0 && (u32)index < _itemCount)
            _activatedItem = index;
    }

    void Close()
    {
        _romBrowserController->HideGuides();
    }

private:
    using GuideName = String<char, kMaxNameLength>;

    IRomBrowserController* _romBrowserController;
    std::unique_ptr<GuideName[]> _items;
    u32 _itemCount = 0;
    u32 _itemCapacity = 0;
    int _initialSelectedItem = 0;
    int _selectedItem = -1;
    int _activatedItem = -1;
};
