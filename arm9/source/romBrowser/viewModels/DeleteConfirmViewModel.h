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
        , _saveFileName(romBrowserController->GetDeleteSaveFileName())
        , _isHide(romBrowserController->IsHideConfirm()) { }

    const char* GetFileName() const { return _fileName.GetString(); }
    const char* GetSaveFileName() const { return _saveFileName.GetString(); }
    bool HasSave() const { return _saveFileName.GetString()[0] != 0; }
    /// @brief The same sheet asks both questions; this is which one.
    bool IsHide() const { return _isHide; }

    void UnhideAll()
    {
        _romBrowserController->UnhideAll();
    }

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
    bool _isHide;
};
