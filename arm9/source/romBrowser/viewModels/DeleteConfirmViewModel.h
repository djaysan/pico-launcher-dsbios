#pragma once
#include "core/String.h"
#include "../IRomBrowserController.h"

/// @brief View model for the delete confirmation sheet. Copies the names at
///        construction time so nothing dangles while the sheet animates.
class DeleteConfirmViewModel
{
public:
    explicit DeleteConfirmViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController)
        , _fileName(romBrowserController->GetDeleteRomFileName())
        , _saveFileName(romBrowserController->GetDeleteSaveFileName()) { }

    const char* GetFileName() const { return _fileName.GetString(); }
    const char* GetSaveFileName() const { return _saveFileName.GetString(); }
    bool HasSave() const { return _saveFileName.GetString()[0] != 0; }

    void Confirm()
    {
        _romBrowserController->ConfirmDelete();
    }

    void Cancel()
    {
        _romBrowserController->CancelDelete();
    }

private:
    IRomBrowserController* _romBrowserController;
    String<char, 256> _fileName;
    String<char, 256> _saveFileName;
};
