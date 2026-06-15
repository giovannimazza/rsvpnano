#pragma once

#include "board/BoardConfig.h"

#if defined(RSVP_BOARD_WAVESHARE_AMOLED_143C)
#include "display/co5300.h"
#define PANEL_INIT co5300Init
#define PANEL_SET_BACKLIGHT co5300SetBacklight
#define PANEL_SET_BRIGHTNESS_PERCENT co5300SetBrightnessPercent
#define PANEL_SLEEP co5300Sleep
#define PANEL_WAKE co5300Wake
#define PANEL_PUSH_COLORS co5300PushColors
#else
#include "display/axs15231b.h"
#define PANEL_INIT axs15231bInit
#define PANEL_SET_BACKLIGHT axs15231bSetBacklight
#define PANEL_SET_BRIGHTNESS_PERCENT axs15231bSetBrightnessPercent
#define PANEL_SLEEP axs15231bSleep
#define PANEL_WAKE axs15231bWake
#define PANEL_PUSH_COLORS axs15231bPushColors
#endif
