#pragma once
#include "SdFolderSortType.h"
#include "SdFolderSortDirection.h"

class IGameDataService;

class SdFolderFilterSortParams
{
public:
    SdFolderSortType sortType = SdFolderSortType::Name;
    SdFolderSortDirection sortDirection = SdFolderSortDirection::Ascending;
    bool includeHiddenFiles = false;
    /// @brief When true only folders and favorite games are kept.
    ///        Requires gameDataService.
    bool favoritesOnly = false;
    /// @brief When true only folders and completed games are kept.
    ///        Requires gameDataService.
    bool completedOnly = false;
    /// @brief When true, folders with no visible entries of their own are
    ///        dropped. Relies on FileInfo::IsEmptyFolder() already being
    ///        populated (see SdFolderFactory::HasVisibleContent) - this flag
    ///        itself does no SD access.
    bool hideEmptyFolders = false;
    const IGameDataService* gameDataService = nullptr;

    SdFolderFilterSortParams() { }

    SdFolderFilterSortParams(SdFolderSortType sortType, SdFolderSortDirection sortDirection, bool includeHiddenFiles)
        : sortType(sortType), sortDirection(sortDirection), includeHiddenFiles(includeHiddenFiles) { }
};
