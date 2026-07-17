#include "common.h"
#include "rtcIpc.h"
#include "ThemeTimeOfDay.h"

namespace ThemeTimeOfDay
{

bool IsNight()
{
    rtc_datetime_t dateTime;
    rtc_readDateTime(&dateTime);
    // the rtc registers hold BCD values
    u32 hour = ((dateTime.time.hour >> 4) * 10) + (dateTime.time.hour & 0xF);
    return hour >= 20 || hour < 7;
}

}
