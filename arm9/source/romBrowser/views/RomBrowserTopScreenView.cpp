#include "common.h"
#include <string.h>
#include <libtwl/mem/memVram.h>
#include <libtwl/gfx/gfx.h>
#include <libtwl/gfx/gfxBackground.h>
#include <libtwl/gfx/gfxPalette.h>
#include <libtwl/gfx/gfxWindow.h>
#include <libtwl/dma/dmaNitro.h>
#include "core/mini-printf.h"
#include "../viewModels/RomBrowserViewModel.h"
#include "../IRomBrowserController.h"
#include "services/gamedata/IGameDataService.h"
#include "gui/GraphicsContext.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "gui/OamBuilder.h"
#include "gui/palette/GradientPalette.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "../Theme/IRomBrowserViewFactory.h"
#include "smallHeartIconFilled.h"
#include "RomBrowserTopScreenView.h"

RomBrowserTopScreenView::RomBrowserTopScreenView(
    SharedPtr<RomBrowserViewModel> viewModel,
    const RomBrowserDisplayMode* displayMode,
    const IThemeFileIconFactory* themeFileIconFactory,
    const IRomBrowserViewFactory* romBrowserViewFactory,
    const IFontRepository* fontRepository,
    const MaterialColorScheme* materialColorScheme)
    : _viewModel(std::move(viewModel))
    , _themeFileIconFactory(themeFileIconFactory)
    , _fileInfoView(romBrowserViewFactory->CreateFileInfoView())
    , _showCover(displayMode->ShowCoverOnTopScreen())
    , _coverPosition(romBrowserViewFactory->GetTopCoverPosition())
{
    AddChildTail(_fileInfoView.GetPointer());

    u32 gameCount = _viewModel->GetFileInfoManager().GetGameCount();
    if (gameCount > 0)
    {
        char text[16];
        mini_snprintf(text, sizeof(text), "%u game%s", gameCount, gameCount == 1 ? "" : "s");
        _gameCountLabel = Label2DView::CreateShared(96, 16, 15, fontRepository->GetFont(FontType::Medium7_5));
        _gameCountLabel->SetText(text);
        // top strip y 0-16: free of theme elements in both themes' defaults
        // (cover starts at y=18, banner text at y>=118, filename at y>=168)
        _gameCountLabel->SetPosition(4, 2);
        _gameCountLabel->SetBackgroundColor(materialColorScheme->surfaceBright);
        _gameCountLabel->SetForegroundColor(materialColorScheme->onSurfaceVariant);
        AddChildTail(_gameCountLabel.GetPointer());
    }

    _gameDataService = _viewModel->GetRomBrowserController()->GetGameDataService();
    _materialColorScheme = materialColorScheme;
    // launch info for the selected game ("3x 16/07"), right-aligned in the top
    // strip, leaving 16px on the right for the favorite heart
    _launchInfoLabel = Label2DView::CreateShared(96, 16, 15, fontRepository->GetFont(FontType::Medium7_5));
    _launchInfoLabel->SetHorizontalAlignment(Alignment::End);
    _launchInfoLabel->SetPosition(256 - 4 - 16 - 96, 2);
    _launchInfoLabel->SetBackgroundColor(materialColorScheme->surfaceBright);
    _launchInfoLabel->SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(_launchInfoLabel.GetPointer());
}

void RomBrowserTopScreenView::InitVram(const VramContext& vramContext)
{
    ViewContainer::InitVram(vramContext);
    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        _heartVramOffset = objVramManager->Alloc(smallHeartIconFilledTilesLen);
        dma_ntrCopy32(3, smallHeartIconFilledTiles,
            objVramManager->GetVramAddress(_heartVramOffset), smallHeartIconFilledTilesLen);
    }
    int tileIndex = 0;
    vu16* mapPtr = (vu16*)((u8*)GFX_BG_SUB + 0x3800);
    for (int y = 0; y < 12; y++)
    {
        for (int x = 0; x < 14; x++)
        {
            *mapPtr++ = tileIndex;
            tileIndex++;
        }
        mapPtr += 2;
    }
}

void RomBrowserTopScreenView::Update()
{
    int selectedItem = _viewModel->GetSelectedItem();
    if (selectedItem != _lastSelectedItem && selectedItem >= 0)
    {
        auto& fileInfoManager = _viewModel->GetFileInfoManager();
        // GetInternalFileInfo() covers both game banners and custom icon overrides (the
        // latter apply to any file type, including folders), so check it directly instead
        // of branching on FileType::HasInternalFileInfo() - that's a static per-type property
        // and knows nothing about a per-item custom icon. IsFileInfoLoaded() distinguishes
        // "still loading" from "loaded, and there's legitimately nothing" so this waits for
        // the io thread instead of flashing the previous item's icon while undecided.
        if (fileInfoManager.IsFileInfoLoaded(selectedItem))
        {
            const auto& item = fileInfoManager.GetItem(selectedItem);
            auto info = fileInfoManager.GetInternalFileInfo(selectedItem);

            bool fileNameAsTitle = true;
            const char16_t* gameTitle = info ? info->GetGameTitle() : nullptr;
            if (gameTitle && gameTitle[0] != 0)
            {
                _fileInfoView->SetGameTitleAsync(_viewModel->GetBgTaskQueue(), gameTitle);
                fileNameAsTitle = false;
            }

            _selectedFileIcon = info ? info->CreateGameIcon() : nullptr;
            if (!_selectedFileIcon)
            {
                _selectedFileIcon = item.GetFileType()->CreateFileIcon("", _themeFileIconFactory);
            }
            if (_selectedFileIcon)
            {
                _selectedFileIcon->SetAnimFrame(_viewModel->GetIconFrameCounter());
                _iconGraphicsUploaded = false;
            }
            _fileInfoView->SetIcon(std::move(_selectedFileIcon));
            _fileInfoView->SetFileNameAsync(_viewModel->GetBgTaskQueue(), item.GetFileName(), fileNameAsTitle);

            _lastSelectedItem = selectedItem;

            auto cover = fileInfoManager.GetFileCover(selectedItem);
            if (cover.IsValid())
            {
                _selectedFileCover = std::move(cover);
                _coverGraphicsUploaded = false;
            }
        }
    }

    u32 gameDataVersion = _gameDataService->GetVersion();
    bool infoLoaded = selectedItem >= 0 && _viewModel->GetFileInfoManager().IsFileInfoLoaded(selectedItem);
    if (selectedItem != _lastGameDataItem || gameDataVersion != _lastGameDataVersion ||
        infoLoaded != _lastGameDataInfoLoaded)
    {
        _selectedFavorite = false;
        char info[24];
        info[0] = 0;
        if (selectedItem >= 0)
        {
            const auto& item = _viewModel->GetFileInfoManager().GetItem(selectedItem);
            const char* gameCode = nullptr;
            if (infoLoaded)
            {
                const auto* internalInfo = _viewModel->GetFileInfoManager().GetInternalFileInfo(selectedItem);
                if (internalInfo)
                    gameCode = internalInfo->GetGameCode();
            }
            const auto* entry = _gameDataService->GetEntry(item.GetFileName(), gameCode);
            if (entry)
            {
                _selectedFavorite = entry->favorite;
                if (entry->launchCount > 0)
                {
                    const char* lastPlayed = entry->lastPlayed.GetString();
                    if (strlen(lastPlayed) >= 10)
                    {
                        // stored as "YYYY-MM-DD HH:MM", shown as "3x 16/07"
                        mini_snprintf(info, sizeof(info), "%ux %c%c/%c%c", entry->launchCount,
                            lastPlayed[8], lastPlayed[9], lastPlayed[5], lastPlayed[6]);
                    }
                    else
                    {
                        mini_snprintf(info, sizeof(info), "%ux", entry->launchCount);
                    }
                }
            }
        }
        _launchInfoLabel->SetText(info);
        _lastGameDataItem = selectedItem;
        _lastGameDataVersion = gameDataVersion;
        _lastGameDataInfoLoaded = infoLoaded;
    }
    ViewContainer::Update();
}

void RomBrowserTopScreenView::Draw(GraphicsContext& graphicsContext)
{
    ViewContainer::Draw(graphicsContext);
    if (_selectedFavorite)
    {
        auto oams = graphicsContext.GetOamManager().AllocOams(1);
        u32 paletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(_materialColorScheme->surfaceBright, _materialColorScheme->primary), 2, 18);
        OamBuilder::OamWithSize<16, 16>(256 - 4 - 16, 2, _heartVramOffset >> 7)
            .WithPalette16(paletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(oams[0]);
    }
}

void RomBrowserTopScreenView::VBlank()
{
    ViewContainer::VBlank();

    if (!_coverGraphicsUploaded && _selectedFileCover.IsValid())
    {
        if (_showCover && _selectedFileCover->IsActualCover())
        {
            _selectedFileCover->Upload2DCoverBitmap((u8*)GFX_BG_SUB + 0x4000);
            mem_setVramHMapping(MEM_VRAM_H_LCDC);
            _selectedFileCover->Upload2DCoverPalette((void*)0x0689E000);
            GFX_PLTT_BG_SUB[0] = *(vu16*)0x0689E000;
            mem_setVramHMapping(MEM_VRAM_H_SUB_BG_EXT_PLTT_SLOT_0123);
        }
        _coverGraphicsUploaded = true;
    }
    int x0 = std::clamp(_coverPosition.x, 0, 256);
    int x1 = std::clamp(_coverPosition.x + 106, 0, 256);
    int y0 = std::clamp(_coverPosition.y, 0, 192);
    int y1 = std::clamp(_coverPosition.y + 96, 0, 192);
    if (!_showCover || !_selectedFileCover.IsValid() || !_selectedFileCover->IsActualCover() ||
        x0 >= x1 || y0 >= y1)
    {
        // hide cover
        REG_DISPCNT_SUB &= ~(((1 << 3) | (1 << 5)) << 8);
    }
    else
    {
        // display cover
        REG_BG3PA_SUB = 0x100;
        REG_BG3PB_SUB = 0;
        REG_BG3PC_SUB = 0;
        REG_BG3PD_SUB = -0x100;
        REG_BG3X_SUB = (-_coverPosition.x) << 8;
        REG_BG3Y_SUB = (96 + _coverPosition.y - 1) << 8;
        REG_BG3CNT_SUB = 0x0705;
        REG_DISPCNT_SUB |= ((1 << 3) | (1 << 5)) << 8;
        gfx_setSubWindow0(x0, y0, x1, y1);
        REG_WININ_SUB = 0x002A;
        REG_WINOUT_SUB = ~(1 << 3);
    }
    if (!_iconGraphicsUploaded)
    {
        _fileInfoView->UploadIconGraphics();
        _iconGraphicsUploaded = true;
    }
}
