#pragma once

namespace ThemeTimeOfDay
{
    /// @brief True between 20:00 and 6:59. Custom theme backgrounds get their
    ///        "_night" variants during those hours (when present).
    bool IsNight();
}
