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

    /// @brief A disabled button is drawn with a faded icon and ignores presses.
    ///        Used for affordances that exist but cannot act right now, e.g. the
    ///        delete button while a folder is highlighted - dimming is honest,
    ///        whereas hiding it would shift every other button in the app bar
    ///        as the selection moves.
    void SetEnabled(bool enabled)
    {
        _enabled = enabled;
    }

    constexpr bool IsEnabled() const { return _enabled; }

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
    bool _enabled = true;

    /// @brief Icon tint: the override when set, the scheme role otherwise.
    Rgb<8, 8, 8> GetIconColor() const;
    Rgb<8, 8, 8> FadeIfDisabled(const Rgb<8, 8, 8>& color) const;

    IconButtonView(Type type, State state,
        md::sys::color backgroundColor, const MaterialColorScheme* materialColorScheme)
        : _iconVramOffset(0), _backgroundColor(backgroundColor)
        , _action(nullptr), _actionArg(nullptr), _type(type), _state(state)
        , _materialColorScheme(materialColorScheme) { }

    bool IsCircleBackgroundVisible() const;
    md::sys::color GetCircleBackgroundColor() const;
    md::sys::color GetForegroundColor() const;

    /// @brief Fill color of the circle behind a focused button. Both the 2D and
    ///        the 3D button share this (the app bar uses one of them and the
    ///        display settings sheet the other, depending on the theme) so focus
    ///        cannot end up looking different in the two places.
    ///
    ///        secondaryContainer normally - the same tone the display settings
    ///        sheet uses for a selected option, so focus reads the same way in
    ///        both places. The exception is a button that is ALREADY selected:
    ///        it draws secondaryContainer at rest (see GetCircleBackgroundColor),
    ///        so focusing it has to move to another tone or the two facts become
    ///        one pixel-identical circle - which left the hide-empty-folders
    ///        toggle with no readable state at all, since pressing it changed
    ///        nothing on screen while it had focus.
    ///        App bar buttons are never selected, so there they are always the
    ///        first tone.
    md::sys::color GetFocusFillColor() const
    {
        return _state == State::ToggleSelected ? md::sys::color::primary
                                               : md::sys::color::secondaryContainer;
    }

    /// @brief Icon tint while focused. An active filter still wins, so a red
    ///        heart or green check keeps saying so; otherwise the icon flips to
    ///        onPrimary, which is the tone guaranteed to be readable on top of
    ///        GetFocusFillColor().
    Rgb<8, 8, 8> GetFocusIconColor() const;
};