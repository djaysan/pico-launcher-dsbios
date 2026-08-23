#include "common.h"
#include "../viewModels/RomBrowserAppBarViewModel.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "backIcon.h"
#include "settingsIcon.h"
#include "heartIcon.h"
#include "recentIcon.h"
#include "guidesIcon.h"
#include "hGridIcon.h"
#include "vGridIcon.h"
#include "bannerListIcon.h"
#include "coverflowIcon.h"
#include "listIcon.h"
#include "gui/IVramManager.h"
#include "../DisplayMode/RomBrowserDisplayMode.h"
#include "RomBrowserAppBarView.h"

RomBrowserAppBarView::RomBrowserAppBarView(
    RomBrowserAppBarViewModel* viewModel, const RomBrowserDisplayMode& displayMode,
    const IRomBrowserViewFactory* romBrowserViewFactory)
    : _viewModel(viewModel)
{
    _appBarView = displayMode.CreateAppBarView(romBrowserViewFactory, 1, 4);
    AddChildTail(_appBarView.GetPointer());

    _appBarView->SetButtonAction(APP_BAR_BUTTON_BACK, [] (IconButtonView* sender, void* arg)
    {
        ((RomBrowserAppBarViewModel*)arg)->NavigateUp();
    }, _viewModel);
    _appBarView->SetButtonAction(APP_BAR_BUTTON_RECENT, [] (IconButtonView* sender, void* arg)
    {
        ((RomBrowserAppBarViewModel*)arg)->ShowRecents();
    }, _viewModel);
    _appBarView->SetButtonAction(APP_BAR_BUTTON_FAVORITE, [] (IconButtonView* sender, void* arg)
    {
        ((RomBrowserAppBarViewModel*)arg)->ToggleFavoritesFilter();
    }, _viewModel);
    // holding the heart opens the cross-folder favorites panel
    _appBarView->SetButtonLongAction(APP_BAR_BUTTON_FAVORITE, [] (IconButtonView* sender, void* arg)
    {
        ((RomBrowserAppBarViewModel*)arg)->ShowFavorites();
    });
    _appBarView->SetButtonAction(APP_BAR_BUTTON_GUIDES, [] (IconButtonView* sender, void* arg)
    {
        ((RomBrowserAppBarViewModel*)arg)->LaunchGuideForSelected();
    }, _viewModel);
    _appBarView->SetButtonAction(APP_BAR_BUTTON_DISPLAY_SETTINGS, [] (IconButtonView* sender, void* arg)
    {
        ((RomBrowserAppBarViewModel*)arg)->ShowDisplaySettings();
    }, _viewModel);
}

void RomBrowserAppBarView::Update()
{
    // the heart tells whether the favorites filter is active
    if (_viewModel->IsFavoritesFilterEnabled())
        _appBarView->SetButtonIconColorOverride(APP_BAR_BUTTON_FAVORITE, Rgb<8, 8, 8>(214, 40, 57));
    else
        _appBarView->ClearButtonIconColorOverride(APP_BAR_BUTTON_FAVORITE);
    // guides is never dimmed: with no guide for the highlighted entry the
    // reader opens its own list, so the button always leads somewhere
    ViewContainer::Update();
}

void RomBrowserAppBarView::InitVram(const VramContext& vramContext)
{
    ViewContainer::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        u32 backIconVramOffset = objVramManager->Alloc(backIconTilesLen);
        dma_ntrCopy32(3, backIconTiles, objVramManager->GetVramAddress(backIconVramOffset), backIconTilesLen);
        _appBarView->SetButtonIcon(APP_BAR_BUTTON_BACK, backIconVramOffset);

        u32 settingsIconVramOffset = objVramManager->Alloc(settingsIconTilesLen);
        dma_ntrCopy32(3, settingsIconTiles, objVramManager->GetVramAddress(settingsIconVramOffset), settingsIconTilesLen);
        _appBarView->SetButtonIcon(APP_BAR_BUTTON_DISPLAY_SETTINGS, settingsIconVramOffset);

        u32 heartIconVramOffset = objVramManager->Alloc(heartIconTilesLen);
        dma_ntrCopy32(3, heartIconTiles, objVramManager->GetVramAddress(heartIconVramOffset), heartIconTilesLen);
        _appBarView->SetButtonIcon(APP_BAR_BUTTON_FAVORITE, heartIconVramOffset);

        u32 recentIconVramOffset = objVramManager->Alloc(recentIconTilesLen);
        dma_ntrCopy32(3, recentIconTiles, objVramManager->GetVramAddress(recentIconVramOffset), recentIconTilesLen);
        _appBarView->SetButtonIcon(APP_BAR_BUTTON_RECENT, recentIconVramOffset);

        u32 guidesIconVramOffset = objVramManager->Alloc(guidesIconTilesLen);
        dma_ntrCopy32(3, guidesIconTiles, objVramManager->GetVramAddress(guidesIconVramOffset), guidesIconTilesLen);
        _appBarView->SetButtonIcon(APP_BAR_BUTTON_GUIDES, guidesIconVramOffset);

        // u32 settingsIconVramOffset = objVramManager->Alloc(settingsIconTilesLen);
        // dma_ntrCopy32(3, settingsIconTiles, objVramManager->GetVramAddress(settingsIconVramOffset), settingsIconTilesLen);
        // _appBarView->SetButtonIcon(APP_BAR_BUTTON_SETTINGS, settingsIconVramOffset);

        // u32 recentIconVramOffset = objVramManager->Alloc(recentIconTilesLen);
        // dma_ntrCopy32(3, recentIconTiles, objVramManager->GetVramAddress(recentIconVramOffset), recentIconTilesLen);
        // _appBarView->SetButtonIcon(APP_BAR_BUTTON_RECENT, recentIconVramOffset);

        // u32 displaySettingsIconVramOffset;
        // switch (_viewModel->GetRomBrowserLayout())
        // {
        //     case RomBrowserLayout::HorizontalIconGrid:
        //     default:
        //     {
        //         displaySettingsIconVramOffset = objVramManager->Alloc(hGridIconTilesLen);
        //         dma_ntrCopy32(3, hGridIconTiles, objVramManager->GetVramAddress(displaySettingsIconVramOffset), hGridIconTilesLen);
        //         break;
        //     }
        //     case RomBrowserLayout::VerticalIconGrid:
        //     {
        //         displaySettingsIconVramOffset = objVramManager->Alloc(vGridIconTilesLen);
        //         dma_ntrCopy32(3, vGridIconTiles, objVramManager->GetVramAddress(displaySettingsIconVramOffset), vGridIconTilesLen);
        //         break;
        //     }
        //     case RomBrowserLayout::BannerList:
        //     {
        //         displaySettingsIconVramOffset = objVramManager->Alloc(bannerListIconTilesLen);
        //         dma_ntrCopy32(3, bannerListIconTiles, objVramManager->GetVramAddress(displaySettingsIconVramOffset), bannerListIconTilesLen);
        //         break;
        //     }
        //     case RomBrowserLayout::FileList:
        //     {
        //         displaySettingsIconVramOffset = objVramManager->Alloc(listIconTilesLen);
        //         dma_ntrCopy32(3, listIconTiles, objVramManager->GetVramAddress(displaySettingsIconVramOffset), listIconTilesLen);
        //         break;
        //     }
        //     case RomBrowserLayout::CoverFlow:
        //     {
        //         displaySettingsIconVramOffset = objVramManager->Alloc(coverflowIconTilesLen);
        //         dma_ntrCopy32(3, coverflowIconTiles, objVramManager->GetVramAddress(displaySettingsIconVramOffset), coverflowIconTilesLen);
        //         break;
        //     }
        // }
        // _appBarView->SetButtonIcon(APP_BAR_BUTTON_DISPLAY_SETTINGS, displaySettingsIconVramOffset);
    }
}

SharedPtr<View> RomBrowserAppBarView::MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
{
    if (!currentFocus)
    {
        return nullptr;
    }
    if (source == _appBarView.GetPointer())
    {
        return View::MoveFocus(currentFocus, direction, source);
    }
    else if (source == GetParent())
    {
        return _appBarView->MoveFocus(currentFocus, direction, this);
    }
    return nullptr;
}
