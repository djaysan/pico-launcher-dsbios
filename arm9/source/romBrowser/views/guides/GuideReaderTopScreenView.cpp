#include "common.h"
#include <libtwl/gfx/gfx.h>
#include <libtwl/gfx/gfxPalette.h>
#include "core/math/ColorConverter.h"
#include "gui/GraphicsContext.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "GuideReaderTopScreenView.h"

#define TEXT_X                      16
#define TEXT_Y                      24
/// Wider than the font's own 11px line: only nine lines fit in sub OBJ vram, so
/// the extra leading fills the screen instead of stranding a block of text at
/// the top - and it reads better at arm's length.
#define TEXT_LINE_HEIGHT            16

GuideReaderTopScreenView::GuideReaderTopScreenView(SharedPtr<GuideReaderViewModel> viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : _viewModel(std::move(viewModel))
    , _materialColorScheme(materialColorScheme)
{
    for (u32 i = 0; i < GuideReaderViewModel::kVisibleLines; i++)
    {
        _lineLabels[i] = Label2DView::CreateShared(kLineWidth, 16,
            GuideReaderViewModel::kMaxLineLength, fontRepository->GetFont(kFontType));
        _lineLabels[i]->SetPosition(TEXT_X, TEXT_Y + i * TEXT_LINE_HEIGHT);
        AddChildTail(_lineLabels[i].GetPointer());
    }
    _savedBackdrop = GFX_PLTT_BG_SUB[0];
    UpdateText();
}

GuideReaderTopScreenView::~GuideReaderTopScreenView()
{
    // the browser's top view only rewrites the backdrop when it uploads a
    // cover, and a game without one never would
    GFX_PLTT_BG_SUB[0] = _savedBackdrop;
}

void GuideReaderTopScreenView::UpdateText()
{
    _shownViewSerial = _viewModel->GetViewSerial();
    u32 lineCount = _viewModel->GetLineCount();
    for (u32 i = 0; i < GuideReaderViewModel::kVisibleLines; i++)
        _lineLabels[i]->SetText(i < lineCount ? _viewModel->GetLine(i) : u"");
    if (!_viewModel->IsOpen())
        _lineLabels[0]->SetText(u"Pick a guide.");
}

void GuideReaderTopScreenView::Update()
{
    // pull, rather than have the sheet push: the serial says whether these
    // labels are stale, so scrolling costs one comparison a frame
    if (_shownViewSerial != _viewModel->GetViewSerial())
        UpdateText();
    ViewContainer::Update();
}

void GuideReaderTopScreenView::Draw(GraphicsContext& graphicsContext)
{
    for (u32 i = 0; i < GuideReaderViewModel::kVisibleLines; i++)
    {
        _lineLabels[i]->SetBackgroundColor(_materialColorScheme->inverseOnSurface);
        _lineLabels[i]->SetForegroundColor(_materialColorScheme->onSurface);
        _lineLabels[i]->Draw(graphicsContext);
    }
}

void GuideReaderTopScreenView::VBlank()
{
    ViewContainer::VBlank();
    // A reader wants a flat page, not theme art with lines of text running
    // across it. Switch off the theme background layer, the cover bitmap and
    // its window, and paint the backdrop in the colour the labels blend their
    // antialiasing to. BOTH background layers: a material theme draws its
    // gradient on BG0_SUB, a bitmap theme (the DS-BIOS "classic" one and every
    // other custom theme) blits topbg.bin to BG2_SUB - clearing only one of
    // them leaves the other showing straight through the text. Runs after the
    // theme background's own VBlank, which is what re-enables its layer every
    // frame; the browser's top view puts all of it back when it returns.
    REG_DISPCNT_SUB &= ~((1 << 8) | (1 << 10) | (1 << 11) | (1 << 13));
    GFX_PLTT_BG_SUB[0] = ColorConverter::ToGBGR565(_materialColorScheme->inverseOnSurface);
    REG_BLDCNT_SUB = 0;
}
