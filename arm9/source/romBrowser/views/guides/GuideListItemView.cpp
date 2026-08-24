#include "common.h"
#include <string.h>
#include "core/StringUtil.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "gui/palette/GradientPalette.h"
#include "gui/GraphicsContext.h"
#include "gui/OamBuilder.h"
#include "gui/input/InputProvider.h"
#include "GuideListItemView.h"

#define NAME_LABEL_X       12
#define NAME_LABEL_Y       5
#define NAME_LABEL_WIDTH   200

GuideListItemView::GuideListItemView(SharedPtr<GuidesViewModel> viewModel, const VramOffsets& vramOffsets,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : _viewModel(std::move(viewModel))
    , _nameLabel(Label2DView::CreateShared(NAME_LABEL_WIDTH, 16, GuidesViewModel::kMaxNameLength,
        fontRepository->GetFont(FontType::Regular10)))
    , _vramOffsets(vramOffsets)
    , _materialColorScheme(materialColorScheme)
{
    _nameLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    AddChildTail(_nameLabel.GetPointer());
}

void GuideListItemView::SetEntry(const char* name, int index)
{
    _index = index;
    // the ".txt" is on every row and only costs width; the folder is the format
    char title[GuidesViewModel::kMaxNameLength + 1];
    StringUtil::Copy(title, name, sizeof(title));
    char* dot = strrchr(title, '.');
    if (dot)
        *dot = 0;
    _nameLabel->SetText(title);
}

void GuideListItemView::Update()
{
    _nameLabel->SetPosition(_position.x + NAME_LABEL_X, _position.y + NAME_LABEL_Y);
    _nameLabel->SetEllipsisStyle(IsFocused()
        ? LabelView::EllipsisStyle::Marquee
        : LabelView::EllipsisStyle::Ellipsis);
    ViewContainer::Update();
}

void GuideListItemView::Draw(GraphicsContext& graphicsContext)
{
    if (!graphicsContext.IsVisible(GetBounds()))
    {
        return;
    }

    auto backColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);
    if (IsFocused())
    {
        auto selectorFullColor = _materialColorScheme->GetColor(md::sys::color::onSurface);
        backColor = RgbMixer::Lerp(backColor, selectorFullColor, 10, 100);

        u32 selectorPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(backColor, backColor),
            _position.y, _position.y + 24);
        auto selectorOam = graphicsContext.GetOamManager().AllocOams(4);
        for (int i = 0; i < 4; i++)
        {
            int x = _position.x + (i < 3 ? i * 64 : 2 * 64 + 32);
            OamBuilder::OamWithSize<64, 32>(x, _position.y, _vramOffsets.selectorVramOffset >> 7)
                .WithPalette16(selectorPaletteRow)
                .WithPriority(graphicsContext.GetPriority())
                .Build(selectorOam[i]);
        }
    }

    _nameLabel->SetBackgroundColor(backColor);
    _nameLabel->SetForegroundColor(_materialColorScheme->onSurface);

    ViewContainer::Draw(graphicsContext);
}

bool GuideListItemView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::A))
    {
        _viewModel->ActivateItem(_index);
        return true;
    }

    return ViewContainer::HandleInput(inputProvider, focusManager);
}

void GuideListItemView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    if (GetBounds().Contains(touchPoint))
    {
        _penDown = true;
    }
}

void GuideListItemView::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    if (!GetBounds().Contains(touchPoint))
    {
        _penDown = false;
    }
}

void GuideListItemView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    if (_penDown && GetBounds().Contains(lastTouchPoint))
    {
        if (focusManager.GetCurrentFocus().GetPointer() == this)
        {
            _viewModel->ActivateItem(_index);
        }
        else
        {
            focusManager.Focus(SharedFromThis());
        }
    }

    _penDown = false;
}
