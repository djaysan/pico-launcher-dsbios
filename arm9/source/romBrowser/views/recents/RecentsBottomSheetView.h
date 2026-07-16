#pragma once
#include <memory>
#include "core/SharedPtr.h"
#include "romBrowser/views/BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "gui/views/RecyclerView.h"
#include "romBrowser/viewModels/RecentsViewModel.h"
#include "RecentsAdapter.h"

class MaterialColorScheme;
class IFontRepository;
class IVramManager;

/// @brief Bottom sheet listing the recently played games.
class RecentsBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(RecentsBottomSheetView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;

    void Focus(FocusManager& focusManager) override;

protected:
    void Close() override;

private:
    SharedPtr<RecentsViewModel> _viewModel;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<Label2DView> _emptyLabel;
    SharedPtr<RecyclerView> _recentsRecycler;
    SharedPtr<RecentsAdapter> _recentsAdapter;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
    IVramManager* _objVramManager = nullptr;
    FocusManager* _focusManager;
    RecentListItemView::VramOffsets _vramOffsets;

    RecentsBottomSheetView(SharedPtr<RecentsViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
        FocusManager* focusManager);

    u32 LoadSprite(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const;
};
