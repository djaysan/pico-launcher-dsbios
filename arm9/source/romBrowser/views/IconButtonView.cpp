#include "common.h"
#include "gui/input/InputProvider.h"
#include "themes/material/MaterialColorScheme.h"
#include "IconButtonView.h"

#define LONG_PRESS_FRAMES   30

bool IconButtonView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::A))
    {
        if (_longAction)
        {
            // the action is decided later: short press fires on release,
            // holding to LONG_PRESS_FRAMES fires the long action instead
            _heldFrames = 1;
        }
        else if (_action)
        {
            _action(this, _actionArg);
        }
        return true;
    }
    else if (_heldFrames > 0 && inputProvider.Current(InputKey::A))
    {
        if (++_heldFrames >= LONG_PRESS_FRAMES)
        {
            _heldFrames = 0; // consumed; the release must not fire the short action
            _longAction(this, _actionArg);
            return true;
        }
        // still deciding: bubble so B/SELECT/START stay responsive under a
        // hold that may yet turn out to be a short press
        return View::HandleInput(inputProvider, focusManager);
    }
    else if (_heldFrames > 0 && inputProvider.Released(InputKey::A))
    {
        _heldFrames = 0;
        if (_action)
        {
            _action(this, _actionArg);
        }
        return true;
    }
    else
    {
        _heldFrames = 0;
        return View::HandleInput(inputProvider, focusManager);
    }
}

void IconButtonView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    if (GetBounds().Contains(touchPoint))
    {
        _penDown = true;
        _penHeldFrames = 0;
    }
}

void IconButtonView::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    if (!GetBounds().Contains(touchPoint))
    {
        _penDown = false;
    }
    else if (_penDown && _longAction)
    {
        if (++_penHeldFrames >= LONG_PRESS_FRAMES)
        {
            _penDown = false; // pen action is complete; the up must not fire short
            focusManager.Focus(SharedFromThis());
            _longAction(this, _actionArg);
        }
    }
}

void IconButtonView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    if (_penDown && GetBounds().Contains(lastTouchPoint))
    {
        focusManager.Focus(SharedFromThis());

        if (_action)
        {
            _action(this, _actionArg);
        }
    }

    _penDown = false;
    _penHeldFrames = 0;
}

bool IconButtonView::IsCircleBackgroundVisible() const
{
    switch (_type)
    {
        case Type::Standard:
        {
            return false;
        }
        case Type::Filled:
        case Type::Tonal:
        {
            return true;
        }
        default:
        {
            // shouldn't happen
            return false;
        }
    }
}

md::sys::color IconButtonView::GetCircleBackgroundColor() const
{
    switch (_type)
    {
        case Type::Standard:
        {
            return _backgroundColor;
        }
        case Type::Filled:
        {
            if (_state == State::ToggleUnselected)
                return md::sys::color::surfaceContainerHighest;
            else
                return md::sys::color::primary;
        }
        case Type::Tonal:
        {
            if (_state == State::ToggleUnselected)
                return md::sys::color::surfaceContainerHighest;
            else
                return md::sys::color::secondaryContainer;
        }
        default:
        {
            // shouldn't happen
            return md::sys::color::onSurfaceVariant;
        }
    }
}

Rgb<8, 8, 8> IconButtonView::GetIconColor() const
{
    return _hasIconColorOverride
        ? _iconColorOverride
        : _materialColorScheme->GetColor(GetForegroundColor());
}

md::sys::color IconButtonView::GetForegroundColor() const
{
    switch (_type)
    {
        case Type::Standard:
        {
            if (_state == State::ToggleSelected)
                return md::sys::color::primary;
            else
                return md::sys::color::onSurfaceVariant;
        }
        case Type::Filled:
        {
            if (_state == State::ToggleUnselected)
                return md::sys::color::primary;
            else
                return md::sys::color::onPrimary;
        }
        case Type::Tonal:
        {
            if (_state == State::ToggleUnselected)
                return md::sys::color::onSurfaceVariant;
            else
                return md::sys::color::onSecondaryContainer;
        }
        default:
        {
            // shouldn't happen
            return md::sys::color::onSurfaceVariant;
        }
    }
}
