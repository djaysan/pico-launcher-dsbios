#pragma once
#include "gui/views/RecyclerAdapter.h"
#include "romBrowser/viewModels/RecentsViewModel.h"
#include "RecentListItemView.h"

/// @brief Recycler adapter for the recents panel.
class RecentsAdapter : public RecyclerAdapter
{
public:
    RecentsAdapter(SharedPtr<RecentsViewModel> recentsViewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
        const RecentListItemView::VramOffsets& vramOffsets)
        : _recentsViewModel(std::move(recentsViewModel)), _materialColorScheme(materialColorScheme)
        , _fontRepository(fontRepository), _vramOffsets(vramOffsets) { }

    u32 GetItemCount() const override
    {
        return _recentsViewModel->GetItemCount();
    }

    void GetViewSize(int& width, int& height) const override
    {
        width = 224;
        height = 24;
    }

    SharedPtr<View> CreateView() const override
    {
        return RecentListItemView::CreateShared(_recentsViewModel, _vramOffsets, _materialColorScheme, _fontRepository);
    }

    void BindView(SharedPtr<View> view, int index) const override
    {
        auto listItemView = static_cast<RecentListItemView*>(view.GetPointer());
        listItemView->SetEntry(&_recentsViewModel->GetItem(index), index);
    }

    void ReleaseView(SharedPtr<View> view, int index) const override
    {
        // Nothing to do
    }

private:
    SharedPtr<RecentsViewModel> _recentsViewModel;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
    RecentListItemView::VramOffsets _vramOffsets;
};
