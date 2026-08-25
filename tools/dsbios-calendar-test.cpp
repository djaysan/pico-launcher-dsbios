// Host self-check for the DS BIOS calendar's date arithmetic, which melonDS can
// never exercise: its emulated rtc reads all zeros and never ticks, so the grid
// on screen there is only ever January 2000.
//
//   g++ -std=c++20 -I arm9/source tools/dsbios-calendar-test.cpp -o /tmp/caltest && /tmp/caltest
#include <cassert>
#include <cstdio>
#include "themes/dsbios/DsBiosCalendar.h"

using namespace DsBiosCalendar;

int main()
{
    // leap years, including the century rules that a naive %4 gets wrong
    assert(IsLeapYear(2024) && !IsLeapYear(2025));
    assert(IsLeapYear(2000) && !IsLeapYear(1900) && !IsLeapYear(2100));
    assert(DaysInMonth(2024, 2) == 29 && DaysInMonth(2025, 2) == 28);
    assert(DaysInMonth(2026, 1) == 31 && DaysInMonth(2026, 4) == 30);
    assert(DaysInMonth(2026, 0) == 0 && DaysInMonth(2026, 13) == 0);

    // 0 = Sunday. Anchors: the reference capture Ludovic supplied shows 18 July
    // 2024 under Th, with the month opening on a Monday.
    assert(DayOfWeek(2024, 7, 18) == 4);
    assert(ColumnOfFirst(2024, 7) == 1);
    // an rtc that never got set reads as 1 Jan 2000, which was a Saturday - this
    // is the one the emulator actually draws, so it is the one to be sure of
    assert(DayOfWeek(2000, 1, 1) == 6);
    // a spread of independently known dates
    assert(DayOfWeek(2026, 8, 25) == 2);   // Tuesday
    assert(DayOfWeek(1999, 12, 31) == 5);  // Friday
    assert(DayOfWeek(2024, 2, 29) == 4);   // Thursday, leap day
    assert(DayOfWeek(2100, 3, 1) == 1);    // Monday, past a skipped leap year

    // five week months versus the six week ones the grid cannot hold whole
    assert(WeeksInMonth(2024, 7) == 5);    // opens Mon, 31 days
    assert(WeeksInMonth(2000, 1) == 6);    // opens Sat, 31 days
    assert(WeeksInMonth(2026, 2) == 4);    // opens Sun, 28 days, exactly four
    assert(FirstVisibleWeek(2024, 7, 18) == 0);
    assert(FirstVisibleWeek(2026, 2, 1) == 0);

    // Jan 2000 opens on a Saturday, so the 1st is alone in week 0 and the 31st
    // lands in week 5. Early in the month the grid holds week 0; once today is
    // past the fifth week it gives that row up so today stays on screen.
    assert(FirstVisibleWeek(2000, 1, 1) == 0);
    assert(FirstVisibleWeek(2000, 1, 29) == 0);   // week 4, still fits
    assert(FirstVisibleWeek(2000, 1, 30) == 1);   // week 5, scroll by one
    assert(FirstVisibleWeek(2000, 1, 31) == 1);

    // whatever the month, today must always land inside the five rows drawn
    for (int year = 1999; year <= 2101; year++)
    {
        for (int month = 1; month <= 12; month++)
        {
            int first = FirstVisibleWeek(year, month, 1);
            assert(first >= 0 && first <= 1);
            for (int day = 1; day <= DaysInMonth(year, month); day++)
            {
                int week = (ColumnOfFirst(year, month) + day - 1) / kColumns;
                int row = week - FirstVisibleWeek(year, month, day);
                assert(row >= 0 && row < kRows);
            }
        }
    }

    printf("dsbios calendar: all checks passed\n");
    return 0;
}
