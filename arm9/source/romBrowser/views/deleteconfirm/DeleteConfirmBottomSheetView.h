#pragma once
#include "core/SharedPtr.h"
#include "romBrowser/views/BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "romBrowser/viewModels/DeleteConfirmViewModel.h"

class MaterialColorScheme;
class IFontRepository;

/// @brief Confirmation sheet before deleting a game (and its save) from the
///        SD card, and — in hide mode — before hiding a folder. X confirms;
///        A and B cancel — A is the launch button and muscle memory must not
///        delete games. In hide mode Y unhides everything in the folder, which
///        is the only route back: a hidden folder is not in the list to press
///        Y on a second time.
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
    /// @brief Whether the second line is used at all: the save warning when
    ///        deleting, the "still on the card" note when hiding.
    bool _showNote = false;

    DeleteConfirmBottomSheetView(SharedPtr<DeleteConfirmViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);
};
