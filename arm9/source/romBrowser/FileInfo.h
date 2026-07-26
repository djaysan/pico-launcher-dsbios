#pragma once
#include "fat/ff.h"
#include "fat/FastFileRef.h"
#include "FileType/FileType.h"

class InternalFileInfo;

class FileInfo
{
public:
    FileInfo() { }
    FileInfo(const FileInfo& fileInfo);
    FileInfo(const TCHAR* fileName, const FileType* type, const FastFileRef& fastFileRef, u8 attributes);

    FileInfo &operator=(FileInfo&& rhs)
    {
        if (this != &rhs)
        {
            _name = std::move(rhs._name);
            _type = rhs._type;
            _fastFileRef = rhs._fastFileRef;
            _emptyFolder = rhs._emptyFolder;
        }

        return *this;
    }

    const TCHAR* GetFileName() const { return _name.get(); }
    const FileType* GetFileType() const { return _type; }
    u32 GetFileSize() const { return _fastFileRef.GetFileSize(); }

    InternalFileInfo* CreateInternalFileInfo() const
    {
        return _type->CreateInternalFileInfo(_fastFileRef);
    }

    const FastFileRef& GetFastFileRef() const { return _fastFileRef; }

    bool IsReadOnly() const { return _attributes & AM_RDO; }
    bool IsHidden() const { return _attributes & AM_HID; }
    bool IsSystem() const { return _attributes & AM_SYS; }

    /// @brief Only meaningful when GetFileType()->GetClassification() is
    ///        Folder: whether this folder has no visible entries of its own.
    ///        Probed once at navigate time (see SdFolderFactory::HasVisibleContent)
    ///        and cached here so re-filtering afterward needs no SD access.
    bool IsEmptyFolder() const { return _emptyFolder; }
    void SetEmptyFolder(bool empty) { _emptyFolder = empty; }

private:
    std::unique_ptr<TCHAR[]> _name;
    const FileType* _type;
    FastFileRef _fastFileRef;
    u8 _attributes;
    bool _emptyFolder = false;
};
