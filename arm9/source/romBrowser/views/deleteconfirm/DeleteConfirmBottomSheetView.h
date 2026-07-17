#pragma once
#include "core/SharedPtr.h"
#include "romBrowser/views/BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "romBrowser/viewModels/DeleteConfirmViewModel.h"

class MaterialColorScheme;
class IFontRepository;

/// @brief Confirmation sheet before deleting a game (and its save) from the
///        SD card. X confirms; A and B cancel — A is the launch button and
///        muscle memory must not delete games.
class DeleteConfirmBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(DeleteConfirmBottomSheetView)

public:
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void Focus(FocusManager& focusManager) override;

protected:
    void Close() override;

private:
    SharedPtr<DeleteConfirmViewModel> _viewModel;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<Label2DView> _fileNameLabel;
    SharedPtr<Label2DView> _saveLabel;
    SharedPtr<Label2DView> _hintLabel;
    const MaterialColorScheme* _materialColorScheme;
    bool _confirmed = false;

    DeleteConfirmBottomSheetView(SharedPtr<DeleteConfirmViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);
};
