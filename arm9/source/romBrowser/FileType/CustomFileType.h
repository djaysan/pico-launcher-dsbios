#pragma once
#include "FileType.h"
#include "services/settings/FileAssociation.h"
#include "../Theme/IThemeFileIconFactory.h"

/// @brief Class representing a custom (user provided) file type.
class CustomFileType : public FileType
{
public:
    CustomFileType()
        : FileType(nullptr, FileTypeClassification::Unknown) { }

    explicit CustomFileType(const FileAssociation* fileAssociation)
        : CustomFileType(fileAssociation, nullptr) { }

    /// @param classification Used only when there is no baseFileType. Rom
    ///        extensions pass Game: guides, favorites, delete, the game count
    ///        and the random pick ALL gate on Game, so a rom left as Misc gets
    ///        none of them - which is why .gba worked (it has a built-in type)
    ///        while .nes and .sfc did not. Non-rom associations (the .txt one
    ///        that opens the guide reader) stay Misc so they are not counted
    ///        or launched as games.
    CustomFileType(const FileAssociation* fileAssociation, const FileType* baseFileType,
        FileTypeClassification classification = FileTypeClassification::Misc)
        : FileType(
            baseFileType != nullptr ? baseFileType->GetShortName() : fileAssociation->extension.GetString(),
            baseFileType != nullptr ? baseFileType->GetClassification() : classification)
        , _fileAssociation(fileAssociation), _baseFileType(baseFileType) { }

    std::unique_ptr<FileIcon> CreateFileIcon(const TCHAR* fileName,
        const IThemeFileIconFactory* themeFileIconFactory) const override
    {
        return _baseFileType != nullptr
            ? _baseFileType->CreateFileIcon(fileName, themeFileIconFactory)
            : themeFileIconFactory->CreateGenericFileIcon(fileName);
    }

    FileCover* CreateFileCover(const TCHAR* fileName) const override
    {
        return _baseFileType != nullptr
            ? _baseFileType->CreateFileCover(fileName)
            : FileType::CreateFileCover(fileName);
    };

    InternalFileInfo* CreateInternalFileInfo(const FastFileRef& fastFileRef) const override
    {
        return _baseFileType != nullptr
            ? _baseFileType->CreateInternalFileInfo(fastFileRef)
            : nullptr;
    }

    bool TrySetLaunchParameters(pload_params_t* launchParameters, const char* filePath) const override
    {
        StringUtil::Copy(launchParameters->romPath, _fileAssociation->applicationPath, sizeof(launchParameters->romPath));
        u32 length = StringUtil::Copy(launchParameters->arguments, filePath, sizeof(launchParameters->arguments));
        launchParameters->argumentsLength = length + 1;
        return true;
    }

private:
    const FileAssociation* _fileAssociation;
    const FileType* _baseFileType = nullptr;
};
