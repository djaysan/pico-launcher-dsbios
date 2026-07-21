#pragma once

/// @brief Sets the DS Lite backlight level (0 = low .. 3 = max) through the
///        ARM7. Fire-and-forget: the ARM7 applies it on its main thread (the
///        PMIC shares the SPI bus with the touch screen) and only after
///        detecting a DS Lite — on the original DS the backlight register
///        mirrors the control register and must not be written. The level
///        persists into the launched game until the console powers off.
void backlight_setLevel(unsigned int level);
