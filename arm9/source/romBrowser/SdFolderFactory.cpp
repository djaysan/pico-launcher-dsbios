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

bool SdFolderFactory::HasVisibleContent(const char* path, bool favoritesOnly, bool completedOnly,
    const IGameDataService* gameDataService) const
{
    Directory directory;
    if (directory.Open(path) != FR_OK)
        return true;

    FILINFO fileInfo;
    while (true)
    {
        if (directory.Read(&fileInfo) != FR_OK)
            return true;

        if (fileInfo.fname[0] == 0)
            return false;

        if (fileInfo.fname[0] == '.' || (fileInfo.fattrib & AM_HID))
            continue;

        auto classification = fileInfo.fattrib & AM_DIR
            ? FileTypeClassification::Folder
            : _fileTypeProvider->GetFileType(fileInfo.fname)->GetClassification();
        if (classification == FileTypeClassification::Unknown)
            continue;

        // mirrors SdFolder::FilterAndSort exactly: favorites/completed only
        // ever exclude non-folder entries, a subfolder always counts
        if (classification != FileTypeClassification::Folder && gameDataService &&
            (favoritesOnly || completedOnly))
        {
            const auto* entry = gameDataService->GetEntry(fileInfo.fname);
            if (favoritesOnly && (!entry || !entry->favorite))
                continue;
            if (completedOnly && (!entry || !entry->completed))
                continue;
        }
        return true;
    }
}