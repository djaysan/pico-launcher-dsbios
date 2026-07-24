#pragma once
#include "core/math/Point.h"

class CustomTopStripElementInfo
{
public:
    CustomTopStripElementInfo(const Point& position, bool hidden)
        : _position(position), _hidden(hidden) { }

    const Point& GetPosition() const { return _position; }
    bool GetIsHidden() const { return _hidden; }

private:
    Point _position;
    bool _hidden;
};
