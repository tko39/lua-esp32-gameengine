#pragma once
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

class LoopExample
{
public:
    LoopExample(TFT_eSPI *tft, XPT2046_Touchscreen *ts);
    void begin();
    void loop();

private:
    TFT_eSPI *tft_;
    XPT2046_Touchscreen *ts_;

    // Ball properties
    float ballX_;
    float ballY_;
    float ballRadius_;
    float velocityX_;
    float velocityY_;

    uint16_t ballColor_;
    uint16_t previousColor_;

    // Touch calibration constants (same defaults as main.cpp)
    // Use names that won't collide with global macros in other files.
    static constexpr int TS_MIN_X_CONST = 240;
    static constexpr int TS_MAX_X_CONST = 3780;
    static constexpr int TS_MIN_Y_CONST = 220;
    static constexpr int TS_MAX_Y_CONST = 3800;

    void updateBallPosition();
    void drawBall(uint16_t color, uint16_t eraseColor);
    void handleTouch();
};
