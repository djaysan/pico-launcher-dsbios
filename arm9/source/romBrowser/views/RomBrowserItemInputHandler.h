#pragma once
#include "core/math/Point.h"

class InputProvider;
class FocusManager;
class IRomBrowserItemViewModel;
class View;

class RomBrowserItemInputHandler
{
public:
    RomBrowserItemInputHandler(View* view, IRomBrowserItemViewModel* viewModel)
        : _view(view), _viewModel(viewModel) { }

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager);
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager);
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager);
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager);

    bool IsPenDown() const { return _penDown; }

private:
    View* _view;
    IRomBrowserItemViewModel* _viewModel;
    bool _penDown = false;
    int _penDownFrames = 0;
    /// @brief Frames X has been held, 0 when no press is pending. Used to
    ///        tell a short press (favorite) from a long press (completed).
    int _xHeldFrames = 0;
    /// @brief Item index the pending X press was armed on; the hold is
    ///        dropped when the pooled view gets rebound to another item.
    int _xArmedItemIndex = -1;
};
