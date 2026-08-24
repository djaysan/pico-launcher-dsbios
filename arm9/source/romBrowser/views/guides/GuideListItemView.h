#pragma once
#include "core/SharedPtr.h"
#include "gui/views/ViewContainer.h"
#include "gui/views/Label2DView.h"
#include "romBrowser/viewModels/GuidesViewModel.h"

class MaterialColorScheme;
class IFontRepository;

/// @brief List item view for the guides sheet: one guide file name.
class GuideListItemView : public ViewContainer
{
    SHARED_ONLY(GuideListItemView)

public:
    struct VramOffsets
    {
        u32 selectorVramOffset = 0;
    };

    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position.x, _position.y, 224, 24);
    }

    void SetEntry(const char* name, int index);

private:
    SharedPtr<GuidesViewModel> _viewModel;
    SharedPtr<Label2DView> _nameLabel;
    VramOffsets _vramOffsets;
    const MaterialColorScheme* _materialColorScheme;
    int _index = -1;
    bool _penDown = false;

    GuideListItemView(SharedPtr<GuidesViewModel> viewModel, const VramOffsets& vramOffsets,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);
};
