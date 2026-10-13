#pragma once
#include <Adafruit_SH110X.h>

#define OLED_SCLK 12
#define OLED_MOSI 11
#define OLED_DC    5
#define OLED_RST   4
#define OLED_CS    6

#define TOUCH_TOP   1
#define TOUCH_LEFT  2
#define TOUCH_RIGHT 3

#define SW 128
#define SH  64
#define FPS 30
#define FRAME_MS (1000 / FPS)

#define BLACK SH110X_BLACK
#define WHITE SH110X_WHITE

extern Adafruit_SH1106G display;