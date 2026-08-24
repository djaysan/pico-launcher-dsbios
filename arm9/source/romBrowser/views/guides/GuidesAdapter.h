#pragma once
#include "gui/views/RecyclerAdapter.h"
#include "romBrowser/viewModels/GuidesViewModel.h"
#include "GuideListItemView.h"

/// @brief Recycler adapter for the guides sheet.
class GuidesAdapter : public RecyclerAdapter
{
public:
    GuidesAdapter(SharedPtr<GuidesViewModel> guidesViewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
        const GuideListItemView::VramOffsets& vramOffsets)
        : _guidesViewModel(std::move(guidesViewModel)), _materialColorScheme(materialColorScheme)
        , _fontRepository(fontRepository), _vramOffsets(vramOffsets) { }

    u32 GetItemCount() const override
    {
        return _guidesViewModel->GetItemCount();
    }

    void GetViewSize(int& width, int& height) const override
    {
        width = 224;
        height = 24;
    }

    SharedPtr<View> CreateView() const override
    {
        return GuideListItemView::CreateShared(_guidesViewModel, _vramOffsets, _materialColorScheme, _fontRepository);
    }

    void BindView(SharedPtr<View> view, int index) const override
    {
        auto listItemView = static_cast<GuideListItemView*>(view.GetPointer());
        listItemView->SetEntry(_guidesViewModel->GetItem(index), index);
    }

    void ReleaseView(SharedPtr<View> view, int index) const override
    {
        // Nothing to do
    }

private:
    SharedPtr<GuidesViewModel> _guidesViewModel;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
    GuideListItemView::VramOffsets _vramOffsets;
};
