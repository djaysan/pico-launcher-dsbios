#include "common.h"
#include "gui/GraphicsContext.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "gui/input/InputProvider.h"
#include "gui/VramContext.h"
#include "gui/palette/GradientPalette.h"
#include "gui/OamBuilder.h"
#include "core/mini-printf.h"
#include "cheatSelector.h"
#include "GuidesBottomSheetView.h"

#define TITLE_LABEL_X               20
#define TITLE_LABEL_Y               16
#define TITLE_LABEL_WIDTH           168

#define PROGRESS_LABEL_X            (TITLE_LABEL_X + TITLE_LABEL_WIDTH + 4)
#define PROGRESS_LABEL_Y            (TITLE_LABEL_Y + 2)
#define PROGRESS_LABEL_WIDTH        44

#define EMPTY_LABEL_X               20
#define EMPTY_LABEL_Y               36

#define LIST_X                      16
#define LIST_Y                      40
#define LIST_WIDTH                  224
#define LIST_HEIGHT                 120

GuidesBottomSheetView::GuidesBottomSheetView(SharedPtr<GuidesViewModel> viewModel,
    SharedPtr<GuideReaderViewModel> reader,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
    FocusManager* focusManager, const char* autoOpenGuideFileName)
    : _viewModel(std::move(viewModel))
    , _reader(std::move(reader))
    , _titleLabel(Label2DView::CreateShared(TITLE_LABEL_WIDTH, 16, 128, fontRepository->GetFont(FontType::Medium11)))
    , _emptyLabel(Label2DView::CreateShared(224, 16, 48, fontRepository->GetFont(FontType::Regular10)))
    , _guidesRecycler(RecyclerView::CreateShared(
        LIST_X, LIST_Y, LIST_WIDTH, LIST_HEIGHT, RecyclerView::Mode::VerticalList))
    , _materialColorScheme(materialColorScheme)
    , _fontRepository(fontRepository)
    , _focusManager(focusManager)
{
    _progressLabel = Label2DView::CreateShared(PROGRESS_LABEL_WIDTH, 16, 8,
        fontRepository->GetFont(FontType::Medium7_5));
    _progressLabel->SetHorizontalAlignment(Alignment::End);
    _titleLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    _titleLabel->SetText(u"Guides");
    _emptyLabel->SetText(u"No guides. Put .txt files in /guides.");
    _emptyLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    AddChildTail(_titleLabel.GetPointer());
    AddChildTail(_progressLabel.GetPointer());
    if (_viewModel->GetItemCount() == 0)
        AddChildTail(_emptyLabel.GetPointer());
    else
        AddChildTail(_guidesRecycler.GetPointer());
    _guidesRecycler->SetWrapAround(true);
    if (autoOpenGuideFileName)
        StartReading(autoOpenGuideFileName);
}

void GuidesBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        _vramOffsets.selectorVramOffset
            = LoadSprite(*objVramManager, cheatSelectorTiles, cheatSelectorTilesLen);
    }

    _objVramManager = vramContext.GetObjVramManager();
}

void GuidesBottomSheetView::Update()
{
    _titleLabel->SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);
    _progressLabel->SetPosition(PROGRESS_LABEL_X, _position.y + PROGRESS_LABEL_Y);
    _emptyLabel->SetPosition(EMPTY_LABEL_X, _position.y + EMPTY_LABEL_Y);
    _guidesRecycler->SetPosition(LIST_X, _position.y + LIST_Y);
    if (_viewModel->GetItemCount() > 0 && !_guidesAdapter && _objVramManager != nullptr)
    {
        _guidesAdapter = SharedPtr<GuidesAdapter>::MakeShared(
            _viewModel, _materialColorScheme, _fontRepository, _vramOffsets);
        _guidesRecycler->SetAdapter(_guidesAdapter, _viewModel->GetInitialSelectedItem());
        _guidesRecycler->InitVram(VramContext(nullptr, _objVramManager, nullptr, nullptr));
        // not while a guide is already open: the d-pad belongs to the reader
        // then, and the list would take it straight back
        if (!_reading)
            _guidesRecycler->Focus(*_focusManager);
    }
    BottomSheetView::Update();
    _viewModel->SetSelectedItem(_guidesRecycler->GetSelectedItem());

    int activated = _viewModel->ConsumeActivatedItem();
    if (activated >= 0)
        StartReading(_viewModel->GetItem((u32)activated));
    if (_reading)
        UpdateHeader();
}

// A guide is picked on the bottom screen and read on the top one, so opening it
// leaves the list exactly where it is - only the d-pad changes hands.
void GuidesBottomSheetView::StartReading(const char* guideFileName)
{
    _reader->Open(guideFileName);
    _reading = true;
    // Focus moves off the list onto a plain child of this sheet, which is what
    // routes the d-pad here (FocusManager::Update turns directions into
    // MoveFocus on the focused view's parent).
    if (_focusManager)
        _focusManager->Focus(_titleLabel->SharedFromThis());
    UpdateHeader();
}

void GuidesBottomSheetView::StopReading()
{
    _reader->SavePosition();
    _reading = false;
    _titleLabel->SetText(u"Guides");
    _progressLabel->SetText(u"");
    Focus(*_focusManager);
}

void GuidesBottomSheetView::UpdateHeader()
{
    _titleLabel->SetText(_reader->GetTitle());
    char progress[8];
    mini_snprintf(progress, sizeof(progress), "%u%%", _reader->GetProgressPercent());
    _progressLabel->SetText(progress);
}

void GuidesBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        auto backColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);

        if (_guidesAdapter)
        {
            graphicsContext.SetClipArea(_guidesRecycler->GetBounds());
            _guidesRecycler->Draw(graphicsContext);
            graphicsContext.SetClipArea(GetBounds());

            // mask strip above the list so scrolled-out rows don't bleed into the title
            auto maskOam = graphicsContext.GetOamManager().AllocOams(4);
            u32 maskPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
                GradientPalette(backColor, backColor),
                _position.y + LIST_Y - 24, _position.y + LIST_Y);
            for (int i = 0; i < 4; i++)
            {
                int x = LIST_X + (i < 3 ? i * 64 : 2 * 64 + 32);
                OamBuilder::OamWithSize<64, 32>(x, _position.y + LIST_Y - 24, _vramOffsets.selectorVramOffset >> 7)
                    .WithPalette16(maskPaletteRow)
                    .WithPriority(graphicsContext.GetPriority())
                    .Build(maskOam[i]);
            }
        }

        _titleLabel->SetBackgroundColor(backColor);
        _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
        _titleLabel->Draw(graphicsContext);

        if (_reading)
        {
            _progressLabel->SetBackgroundColor(backColor);
            _progressLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
            _progressLabel->Draw(graphicsContext);
        }

        if (_viewModel->GetItemCount() == 0)
        {
            _emptyLabel->SetBackgroundColor(backColor);
            _emptyLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
            _emptyLabel->Draw(graphicsContext);
        }
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

void GuidesBottomSheetView::Focus(FocusManager& focusManager)
{
    if (_reading)
    {
        focusManager.Focus(_titleLabel->SharedFromThis());
    }
    else if (_viewModel->GetItemCount() > 0)
    {
        _guidesRecycler->Focus(focusManager);
    }
    else
    {
        // an empty sheet must still capture key input (B to close). Focus a
        // CHILD of the sheet: FocusManager::Update skips parent-less focused
        // views, so focusing the sheet itself would never deliver keys.
        focusManager.Focus(_titleLabel->SharedFromThis());
    }
}

// The d-pad never reaches HandleInput: FocusManager::Update turns directions
// into MoveFocus calls before anything else sees them. While a guide is open
// there is nothing to move focus to, so the direction scrolls the guide instead
// and the focus stays put (returning null leaves it alone).
SharedPtr<View> GuidesBottomSheetView::MoveFocus(const SharedPtr<View>& currentFocus,
    FocusMoveDirection direction, View* source)
{
    if (!_reading)
        return BottomSheetView::MoveFocus(currentFocus, direction, source);

    if (direction == FocusMoveDirection::Down || direction == FocusMoveDirection::Right)
        _reader->ScrollDown();
    else
        _reader->ScrollUp();
    return nullptr;
}

bool GuidesBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        // one step at a time: out of the guide, then out of the sheet
        if (_reading)
            StopReading();
        else
            _viewModel->Close();
        return true;
    }
    if (_reading)
    {
        // a screenful at a time, for crossing a long guide
        if (inputProvider.Triggered(InputKey::R))
        {
            _reader->PageDown();
            return true;
        }
        if (inputProvider.Triggered(InputKey::L))
        {
            _reader->PageUp();
            return true;
        }
    }
    return false;
}

void GuidesBottomSheetView::Close()
{
    _reader->SavePosition();
    _viewModel->Close();
}

u32 GuidesBottomSheetView::LoadSprite(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const
{
    u32 vramOffset = vramManager.Alloc(tilesLength);
    dma_ntrCopy32(3, tiles, vramManager.GetVramAddress(vramOffset), tilesLength);
    return vramOffset;
}
