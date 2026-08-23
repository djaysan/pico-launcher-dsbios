#include "common.h"
#include "romBrowser/IRomBrowserController.h"
#include "RomBrowserViewModel.h"
#include "romBrowser/FileType/Nds/NdsFileType.h"
#include "RomBrowserItemViewModel.h"

void RomBrowserItemViewModel::Activate()
{
    if (_index >= 0)
    {
        auto& fileInfoManager = _romBrowserController->GetRomBrowserViewModel()->GetFileInfoManager();
        const auto& item = fileInfoManager.GetItem(_index);
        if (item.GetFileType()->GetClassification() == FileTypeClassification::Folder)
        {
            _romBrowserController->NavigateToPath(item.GetFileName());
        }
        else
        {
            _romBrowserController->LaunchFile(item, GetGameCode(fileInfoManager));
        }
    }
}

void RomBrowserItemViewModel::ToggleFavorite()
{
    if (_index >= 0)
    {
        auto& fileInfoManager = _romBrowserController->GetRomBrowserViewModel()->GetFileInfoManager();
        const auto& item = fileInfoManager.GetItem(_index);
        if (item.GetFileType()->GetClassification() == FileTypeClassification::Game)
        {
            _romBrowserController->ToggleFavorite(item, GetGameCode(fileInfoManager));
        }
    }
}

void RomBrowserItemViewModel::RequestDelete()
{
    if (_index < 0)
        return;
    // The long press happens on THIS item, but RequestDeleteSelected works on
    // the browser's selection. They track each other, so this guard should
    // never fire - but deleting a rom is the one action in here that cannot be
    // undone, so it refuses rather than assumes. CanDeleteSelected then keeps
    // folders and support files out of it.
    const auto& viewModel = _romBrowserController->GetRomBrowserViewModel();
    if (!viewModel.IsValid() || viewModel->GetSelectedItem() != _index)
        return;
    if (_romBrowserController->CanDeleteSelected())
    {
        _romBrowserController->RequestDeleteSelected();
    }
}

const char* RomBrowserItemViewModel::GetGameCode(FileInfoManager& fileInfoManager) const
{
    if (!fileInfoManager.IsFileInfoLoaded(_index))
        return nullptr;
    const auto* info = fileInfoManager.GetInternalFileInfo(_index);
    return info ? info->GetGameCode() : nullptr;
}

void RomBrowserItemViewModel::ShowGameInfo()
{
    if (_index >= 0)
    {
        const auto& item = _romBrowserController->GetRomBrowserViewModel()->GetFileInfoManager().GetItem(_index);
        if (item.GetFileType() == &NdsFileType::sInstance)
        {
            _romBrowserController->ShowGameInfo(item);
        }
    }
}
