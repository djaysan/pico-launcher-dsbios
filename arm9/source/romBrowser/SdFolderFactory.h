#pragma once
#include <memory>
#include "SdFolder.h"
#include "FileType/IFileTypeProvider.h"

class IGameDataService;

class SdFolderFactory
{
public:
    SdFolderFactory(const IFileTypeProvider* fileTypeProvider)
        : _fileTypeProvider(fileTypeProvider) { }

    std::unique_ptr<SdFolder> CreateFromPath(const char* path) const;

    /// @brief Cheap, allocation-free check for whether the folder at \p path
    ///        has at least one visible entry under the SAME rule
    ///        SdFolder::FilterAndSort applies: not hidden, not
    ///        FileTypeClassification::Unknown, and - for non-folder entries
    ///        only, exactly like FilterAndSort - subject to \p favoritesOnly /
    ///        \p completedOnly via \p gameDataService. Without this, a folder
    ///        full of non-favorite games would be reported as non-empty while
    ///        the favorites-only filter empties it in the actual listing.
    ///        Stops at the first match instead of reading the whole directory
    ///        like CreateFromPath does, so probing a folder full of games
    ///        stays cheap - only a folder that turns out empty pays for a
    ///        full read. On failure to open/read the folder, returns true
    ///        (fail open: never hide a folder we couldn't actually inspect).
    bool HasVisibleContent(const char* path, bool favoritesOnly, bool completedOnly,
        const IGameDataService* gameDataService) const;

private:
    const IFileTypeProvider* _fileTypeProvider;
};