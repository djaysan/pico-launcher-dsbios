#include "common.h"
#include <vector>
#include "fat/Directory.h"
#include "FileInfo.h"
#include "FileType/Folder/FolderFileType.h"
#include "services/gamedata/IGameDataService.h"
#include "SdFolderFactory.h"

std::unique_ptr<SdFolder> SdFolderFactory::CreateFromPath(const char* path) const
{
    Directory directory;
    if (directory.Open(path) != FR_OK)
        return nullptr;

    int count = 0;
    int bufferSize = 8;
    auto fileInfos = (FileInfo**)malloc(sizeof(FileInfo*) * bufferSize);
    auto sdFileInfo = std::make_unique<FILINFO>();
    while (true)
    {
        if (directory.Read(sdFileInfo.get()) != FR_OK)
            return nullptr;

        if (sdFileInfo->fname[0] == 0)
            break;

        if (count >= bufferSize)
        {
            bufferSize *= 2;
            fileInfos = (FileInfo**)realloc(fileInfos, sizeof(FileInfo*) * bufferSize);
        }
        auto fileType = sdFileInfo->fattrib & AM_DIR
            ? &FolderFileType::sInstance
            : _fileTypeProvider->GetFileType(sdFileInfo->fname);
        fileInfos[count++] = new FileInfo(sdFileInfo->fname, fileType,
            FastFileRef(directory.GetFatFsDirectory(), sdFileInfo.get()), sdFileInfo->fattrib);
    }

    return std::make_unique<SdFolder>(fileInfos, count);
}

namespace
{
    // Kept small on purpose: each level of HasVisibleContentBounded's
    // recursion runs on the IO task's own small fixed stack, alongside the
    // navigate lambda's locals that are already resident for the whole probe
    // loop (see RomBrowserController::HandleNavigateTrigger). A deep chain
    // here previously stacked a fresh FILINFO (~300 bytes) and a 256-byte
    // path buffer PER LEVEL, which could overrun that stack well before
    // hitting the old depth cap of 8 - silent corruption, not just a slow
    // scan, and a much more likely explanation for a hang bad enough to need
    // a power cycle. HasVisibleContentBounded below shares one FILINFO and
    // one path buffer across every level instead, so 4 is now real headroom,
    // not a tight fit - and no realistic manually-nested empty-folder chain
    // goes deeper than this anyway.
    constexpr int kMaxDepth = 4;
}

bool SdFolderFactory::HasVisibleContent(const char* path, bool favoritesOnly, bool completedOnly,
    const IGameDataService* gameDataService, int& readBudget) const
{
    // an earlier sibling folder in this navigation may have already spent
    // the whole shared budget - fail open without opening anything
    if (readBudget <= 0)
        return true;

    const char* baseName = strrchr(path, '/');
    baseName = baseName ? baseName + 1 : path;
    if (baseName[0] == '_')
        return true;

    char pathBuffer[256];
    size_t pathLength = strlen(path);
    if (pathLength >= sizeof(pathBuffer))
        return true; // fail open: can't even hold the starting path
    memcpy(pathBuffer, path, pathLength + 1);

    // shared by reference across the whole recursion below, so every level
    // reuses this one instance instead of stacking its own - see kMaxDepth
    FILINFO fileInfo;
    return HasVisibleContentBounded(pathBuffer, sizeof(pathBuffer), pathLength,
        favoritesOnly, completedOnly, gameDataService, fileInfo, readBudget, kMaxDepth);
}

bool SdFolderFactory::HasVisibleContentBounded(char* pathBuffer, size_t pathBufferSize, size_t pathLength,
    bool favoritesOnly, bool completedOnly, const IGameDataService* gameDataService,
    FILINFO& fileInfo, int& readBudget, int depth) const
{
    // guard before opening too, not just inside the loop below - otherwise an
    // already-exhausted budget still pays for one real Directory::Open() per
    // recursion level on the way down before the loop's own check catches it
    if (readBudget <= 0)
        return true;

    Directory directory;
    if (directory.Open(pathBuffer) != FR_OK)
        return true;

    while (true)
    {
        if (readBudget-- <= 0)
            return true; // fail open: budget exhausted, assume non-empty

        if (directory.Read(&fileInfo) != FR_OK)
            return true;

        if (fileInfo.fname[0] == 0)
            return false;

        if (fileInfo.fname[0] == '.' || (fileInfo.fattrib & AM_HID))
            continue;

        bool isFolder = fileInfo.fattrib & AM_DIR;
        auto classification = isFolder
            ? FileTypeClassification::Folder
            : _fileTypeProvider->GetFileType(fileInfo.fname)->GetClassification();
        if (classification == FileTypeClassification::Unknown)
            continue;

        // mirrors SdFolder::FilterAndSort exactly: favorites/completed only
        // ever exclude non-folder entries
        if (!isFolder && gameDataService && (favoritesOnly || completedOnly))
        {
            const auto* entry = gameDataService->GetEntry(fileInfo.fname);
            if (favoritesOnly && (!entry || !entry->favorite))
                continue;
            if (completedOnly && (!entry || !entry->completed))
                continue;
        }

        // a subfolder only counts if IT has visible content too, so a chain
        // of nested empty folders is fully hidden, not just its outer layer.
        // Underscore-prefixed folders (_pico, _gba) are the launcher's own
        // support data (matches upstream issue #45, "filter out files/folders
        // starting with '_'") - skip them for free instead of spending the
        // shared read budget on hundreds of cover/theme/emulator files. (The
        // top-level path itself gets the same check in HasVisibleContent,
        // since _pico/_gba are always probed directly, never found nested
        // inside another probed folder on a real card.)
        if (isFolder)
        {
            if (fileInfo.fname[0] == '_')
                return true;
            if (depth <= 0)
                return true; // fail open: assume non-empty past the depth cap

            // append "/name" onto the shared buffer in place and back it out
            // after recursing, instead of stacking a fresh path buffer per
            // level - see kMaxDepth for why that stacking mattered
            size_t nameLength = strlen(fileInfo.fname);
            if (pathLength + 1 + nameLength >= pathBufferSize)
                return true; // fail open: path too long to safely descend into
            pathBuffer[pathLength] = '/';
            memcpy(pathBuffer + pathLength + 1, fileInfo.fname, nameLength + 1);
            bool childHasContent = HasVisibleContentBounded(pathBuffer, pathBufferSize,
                pathLength + 1 + nameLength, favoritesOnly, completedOnly, gameDataService,
                fileInfo, readBudget, depth - 1);
            pathBuffer[pathLength] = 0;
            if (!childHasContent)
                continue;
        }
        return true;
    }
}