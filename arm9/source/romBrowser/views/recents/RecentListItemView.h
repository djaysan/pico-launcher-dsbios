#pragma once
#include "core/SharedPtr.h"
#include "gui/views/ViewContainer.h"
#include "gui/views/Label2DView.h"
#include "romBrowser/viewModels/RecentsViewModel.h"

class MaterialColorScheme;
class IFontRepository;

/// @brief List item view for the recents panel: game name plus last played
///        date, with a heart icon for favorites.
class RecentListItemView : public ViewContainer
{
    SHARED_ONLY(RecentListItemView)

public:
    struct VramOffsets
    {
        u32 selectorVramOffset = 0;
        u32 heartIconVramOffset = 0;
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

    void SetEntry(const GameDataEntry* entry, int index);

private:
    SharedPtr<RecentsViewModel> _viewModel;
    SharedPtr<Label2DView> _nameLabel;
    SharedPtr<Label2DView> _dateLabel;
    VramOffsets _vramOffsets;
    const MaterialColorScheme* _materialColorScheme;
    const GameDataEntry* _entry = nullptr;
    int _index = -1;
    bool _penDown = false;

    RecentListItemView(SharedPtr<RecentsViewModel> viewModel, const VramOffsets& vramOffsets,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);
};
