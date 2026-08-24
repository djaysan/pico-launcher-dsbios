#pragma once
#include "core/SharedPtr.h"
#include "gui/views/ViewContainer.h"
#include "gui/views/Label2DView.h"
#include "themes/FontType.h"
#include "romBrowser/viewModels/GuideReaderViewModel.h"

class MaterialColorScheme;
class IFontRepository;

/// @brief The guide itself, on the top screen, while the list stays on the
///        bottom. Replaces the browser's top screen view for as long as the
///        guides sheet is up - it needs the whole 16 KB of sub OBJ vram.
class GuideReaderTopScreenView : public ViewContainer
{
    SHARED_ONLY(GuideReaderTopScreenView)

public:
    /// @brief Width the text wraps to.
    static constexpr u32 kLineWidth = 224;
    /// @brief Font the reader measures and draws with. The view model has to
    ///        wrap with exactly the font the labels render.
    static constexpr FontType kFontType = FontType::Medium7_5;

    ~GuideReaderTopScreenView() override;

    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    void VBlank() override;

    Rectangle GetBounds() const override
    {
        return Rectangle(0, 0, 256, 192);
    }

private:
    SharedPtr<GuideReaderViewModel> _viewModel;
    SharedPtr<Label2DView> _lineLabels[GuideReaderViewModel::kVisibleLines];
    const MaterialColorScheme* _materialColorScheme;
    u32 _shownViewSerial = 0;
    u16 _savedBackdrop = 0;

    GuideReaderTopScreenView(SharedPtr<GuideReaderViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);

    void UpdateText();
};
