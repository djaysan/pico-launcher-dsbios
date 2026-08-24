#pragma once
#include <memory>
#include "core/SharedPtr.h"
#include "romBrowser/views/BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "gui/views/RecyclerView.h"
#include "romBrowser/viewModels/GuidesViewModel.h"
#include "romBrowser/viewModels/GuideReaderViewModel.h"
#include "GuidesAdapter.h"

class MaterialColorScheme;
class IFontRepository;
class IVramManager;

/// @brief Bottom sheet listing every guide in /guides.
class GuidesBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(GuidesBottomSheetView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;

    void Focus(FocusManager& focusManager) override;

protected:
    void Close() override;

private:
    SharedPtr<GuidesViewModel> _viewModel;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<Label2DView> _emptyLabel;
    SharedPtr<RecyclerView> _guidesRecycler;
    SharedPtr<GuidesAdapter> _guidesAdapter;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
    IVramManager* _objVramManager = nullptr;
    FocusManager* _focusManager;
    GuideListItemView::VramOffsets _vramOffsets;

    /// @brief Set while a guide is being read: the d-pad scrolls the top
    ///        screen instead of moving the list, and B steps back to the list.
    bool _reading = false;
    SharedPtr<GuideReaderViewModel> _reader;
    SharedPtr<Label2DView> _progressLabel;

    /// @param autoOpenGuideFileName Guide to start reading straight away (the
    ///        highlighted game's own), or nullptr to wait for a pick.
    GuidesBottomSheetView(SharedPtr<GuidesViewModel> viewModel,
        SharedPtr<GuideReaderViewModel> reader,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
        FocusManager* focusManager, const char* autoOpenGuideFileName);

    void StartReading(const char* guideFileName);
    void StopReading();
    void UpdateHeader();

    u32 LoadSprite(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const;
};
