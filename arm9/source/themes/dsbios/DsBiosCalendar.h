#pragma once

/// @brief The calendar's date arithmetic, kept free of every DS header so the
///        host test in tools/dsbios-calendar-test.cpp can include it directly.
///        The rtc cannot be trusted to tell us a weekday - its weekDay field is
///        a bare counter whose zero point is user defined - so it is computed.
namespace DsBiosCalendar
{

// The grid is 7 columns by 5 rows, which is the original's, and 5 rows is not
// always enough: a month whose 1st falls late enough spills into a 6th week.
constexpr int kColumns = 7;
constexpr int kRows = 5;

inline bool IsLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

inline int DaysInMonth(int year, int month)
{
    static const int sDays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month < 1 || month > 12)
        return 0;
    if (month == 2 && IsLeapYear(year))
        return 29;
    return sDays[month - 1];
}

/// @brief Day of week for a date, 0 = Sunday, matching the grid's first column.
///        Sakamoto's method; valid for any date after 1752.
inline int DayOfWeek(int year, int month, int day)
{
    static const int sOffsets[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    if (month < 3)
        year--;
    return (year + year / 4 - year / 100 + year / 400 + sOffsets[month - 1] + day) % 7;
}

/// @brief Which grid column the 1st of the month sits in.
inline int ColumnOfFirst(int year, int month)
{
    return DayOfWeek(year, month, 1);
}

/// @brief How many weeks the month needs; 4 to 6.
inline int WeeksInMonth(int year, int month)
{
    int cells = ColumnOfFirst(year, month) + DaysInMonth(year, month);
    return (cells + kColumns - 1) / kColumns;
}

/// @brief The week the grid starts on, so that today is always one of the five
///        rows drawn. Months that fit in five weeks always start at week 0; a
///        six week month gives up its first row - whose leading cells are blank
///        anyway - only once today has moved past the fifth.
inline int FirstVisibleWeek(int year, int month, int day)
{
    int overflow = WeeksInMonth(year, month) - kRows;
    if (overflow <= 0)
        return 0;
    int weekOfDay = (ColumnOfFirst(year, month) + day - 1) / kColumns;
    int start = weekOfDay - (kRows - 1);
    if (start < 0)
        start = 0;
    if (start > overflow)
        start = overflow;
    return start;
}

}
