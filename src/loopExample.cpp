#include "loopExample.hpp"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>

LoopExample::LoopExample(TFT_eSPI *tft, XPT2046_Touchscreen *ts)
    : tft_(tft), ts_(ts), ballX_(0), ballY_(0), ballRadius_(20.0f), velocityX_(4.0f), velocityY_(3.5f),
      ballColor_(TFT_WHITE), previousColor_(TFT_BLACK)
{
}

void LoopExample::begin()
{
    // center the ball
    ballX_ = tft_->width() / 2.0f;
    ballY_ = tft_->height() / 2.0f;
}

void LoopExample::loop()
{
    handleTouch();
    drawBall(TFT_BLACK, previousColor_);
    updateBallPosition();
    drawBall(ballColor_, TFT_BLACK);
    previousColor_ = ballColor_;
    delay(40);
}

void LoopExample::drawBall(uint16_t color, uint16_t eraseColor)
{
    if (color != eraseColor)
    {
        tft_->fillCircle((int)ballX_, (int)ballY_, (int)ballRadius_, color);
    }
}

void LoopExample::updateBallPosition()
{
    ballX_ += velocityX_;
    ballY_ += velocityY_;

    int minX = (int)ballRadius_;
    int maxX = tft_->width() - (int)ballRadius_;
    int minY = (int)ballRadius_;
    int maxY = tft_->height() - (int)ballRadius_;

    if (ballX_ < minX)
    {
        ballX_ = minX;
        velocityX_ = -velocityX_;
    }
    else if (ballX_ > maxX)
    {
        ballX_ = maxX;
        velocityX_ = -velocityX_;
    }

    if (ballY_ < minY)
    {
        ballY_ = minY;
        velocityY_ = -velocityY_;
    }
    else if (ballY_ > maxY)
    {
        ballY_ = maxY;
        velocityY_ = -velocityY_;
    }
}

void LoopExample::handleTouch()
{
    if (ts_->tirqTouched() && ts_->touched())
    {
        TS_Point p = ts_->getPoint();

        int pixelX = map(p.x, TS_MIN_X_CONST, TS_MAX_X_CONST, 0, tft_->width());
        int pixelY = map(p.y, TS_MIN_Y_CONST, TS_MAX_Y_CONST, 0, tft_->height());
        float distance = sqrt(pow(pixelX - ballX_, 2) + pow(pixelY - ballY_, 2));

        if (distance <= ballRadius_ * 2)
        {
            switch (ballColor_)
            {
            case TFT_RED:
                ballColor_ = TFT_GREEN;
                break;
            case TFT_GREEN:
                ballColor_ = TFT_BLUE;
                break;
            case TFT_BLUE:
                ballColor_ = TFT_YELLOW;
                break;
            case TFT_YELLOW:
                ballColor_ = TFT_MAGENTA;
                break;
            case TFT_MAGENTA:
                ballColor_ = TFT_CYAN;
                break;
            case TFT_CYAN:
            default:
                ballColor_ = TFT_RED;
                break;
            }
        }
    }
}
