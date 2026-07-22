#pragma once
#include "gui/views/View.h"
#include "gui/materialDesign.h"
#include "core/math/Rgb.h"

class MaterialColorScheme;
class IVramManager;

class IconButtonView : public View
{
public:
    typedef void (*button_action_t)(IconButtonView* sender, void* arg);

    enum class Type
    {
        Standard,
        Filled,
        Tonal
    };

    enum class State
    {
        NoToggle,
        ToggleUnselected,
        ToggleSelected
    };

    void SetIconVramOffset(u32 vramOffset) { _iconVramOffset = vramOffset; }

    Rectangle GetBounds() const override
    {
        return Rectangle(_position, 32, 32);
    }

    void SetAction(button_action_t action, void* arg)
    {
        _action = action;
        _actionArg = arg;
    }

    /// @brief Optional second action fired by holding the button (A or pen)
    ///        for ~half a second. Setting it moves the short action to the
    ///        release edge, so a long press never also fires the short one.
    void SetLongAction(button_action_t longAction)
    {
        _longAction = longAction;
    }

    void SetState(State state)
    {
        _state = state;
    }

    /// @brief Overrides the icon tint, e.g. to signal an active filter.
    void SetIconColorOverride(const Rgb<8, 8, 8>& color)
    {
        _iconColorOverride = color;
        _hasIconColorOverride = true;
    }

    void ClearIconColorOverride()
    {
        _hasIconColorOverride = false;
    }

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

    void SetFocused(bool focused) override
    {
        // a pending A-hold is only valid while this button keeps focus:
        // losing focus mid-hold must not let a later refocus resume the count
        if (!focused)
            _heldFrames = 0;
        View::SetFocused(focused);
    }

protected:
    u32 _iconVramOffset;
    md::sys::color _backgroundColor;
    button_action_t _action;
    button_action_t _longAction = nullptr;
    void* _actionArg;
    Type _type;
    State _state;
    const MaterialColorScheme* _materialColorScheme;
    bool _penDown = false;
    /// @brief Frames the A button has been held, 0 when no press is pending.
    ///        Only counted when a long action is set. Separate from the pen
    ///        counter: HandleInput runs every focused frame and resets this,
    ///        which must not disturb an in-progress pen hold.
    int _heldFrames = 0;
    /// @brief Frames the pen has been held on the button, 0 when none.
    int _penHeldFrames = 0;
    Rgb<8, 8, 8> _iconColorOverride;
    bool _hasIconColorOverride = false;

    /// @brief Icon tint: the override when set, the scheme role otherwise.
    Rgb<8, 8, 8> GetIconColor() const;

    IconButtonView(Type type, State state,
        md::sys::color backgroundColor, const MaterialColorScheme* materialColorScheme)
        : _iconVramOffset(0), _backgroundColor(backgroundColor)
        , _action(nullptr), _actionArg(nullptr), _type(type), _state(state)
        , _materialColorScheme(materialColorScheme) { }

    bool IsCircleBackgroundVisible() const;
    md::sys::color GetCircleBackgroundColor() const;
    md::sys::color GetForegroundColor() const;

    /// @brief Base color of the focus/pressed circle. Standard buttons draw
    ///        nothing at rest and sit on the app bar (their _backgroundColor),
    ///        so a focus tint of that base is invisible; use the same distinct
    ///        surface tone the tonal buttons carry (e.g. the display-settings
    ///        sheet) so the highlight reads clearly. Tonal/filled keep their
    ///        own resting circle color.
    md::sys::color GetFocusCircleColor() const
    {
        return _type == Type::Standard ? md::sys::color::surfaceContainerHighest
                                       : GetCircleBackgroundColor();
    }
};