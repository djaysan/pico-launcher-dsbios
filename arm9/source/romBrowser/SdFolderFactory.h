#pragma once
#include <memory>
#include "fat/Directory.h"
#include "SdFolder.h"
#include "FileType/IFileTypeProvider.h"

class IGameDataService;

class SdFolderFactory
{
public:
    SdFolderFactory(const IFileTypeProvider* fileTypeProvider)
        : _fileTypeProvider(fileTypeProvider) { }

    std::unique_ptr<SdFolder> CreateFromPath(const char* path) const;

    /// @brief Starting value callers should give \p readBudget in
    ///        HasVisibleContent. Pass the SAME variable to every folder
    ///        probed during one navigation (see RomBrowserController) so the
    ///        allowance is shared across all of them, not handed out fresh
    ///        per folder - a folder full of non-game files (hundreds of
    ///        photos, or launcher cover art under _pico) can then only ever
    ///        cost this many directory-entry reads in total for the whole
    ///        navigation.
    static constexpr int kInitialReadBudget = 300;

    /// @brief Cheap, allocation-free check for whether the folder at \p path
    ///        has at least one visible entry under the SAME rule
    ///        SdFolder::FilterAndSort applies: not hidden, not
    ///        FileTypeClassification::Unknown, and - for non-folder entries
    ///        only, exactly like FilterAndSort - subject to \p favoritesOnly /
    ///        \p completedOnly via \p gameDataService. Without this, a folder
    ///        full of non-favorite games would be reported as non-empty while
    ///        the favorites-only filter empties it in the actual listing.
    ///        A subfolder only counts as content if IT has visible content
    ///        (recurses, bounded by a small depth cap - see the .cpp for why
    ///        it's kept small: this runs on the IO task's small fixed stack).
    ///        \p readBudget is consumed for every directory entry actually
    ///        read; once it reaches zero (here, or already from an earlier
    ///        sibling folder sharing the same variable), this and every
    ///        following call fails open with no further I/O at all. \p path
    ///        itself, and any folder named starting with '_' found along the
    ///        way, is the launcher's own support data (matches upstream issue
    ///        #45) and is skipped for free without spending any budget on it.
    ///        Stops at the first match instead of reading the whole directory
    ///        like CreateFromPath does, so probing a folder full of games
    ///        stays cheap - only an actually-empty chain pays for the extra
    ///        reads. On failure to open/read a folder, once the budget runs
    ///        out, or past the depth cap, returns true (fail open: never hide
    ///        something we couldn't actually inspect).
    bool HasVisibleContent(const char* path, bool favoritesOnly, bool completedOnly,
        const IGameDataService* gameDataService, int& readBudget) const;

private:
    bool HasVisibleContentBounded(char* pathBuffer, size_t pathBufferSize, size_t pathLength,
        bool favoritesOnly, bool completedOnly, const IGameDataService* gameDataService,
        FILINFO& fileInfo, int& readBudget, int depth) const;

    const IFileTypeProvider* _fileTypeProvider;
};