#include "MainEngine.h"

MainEngine::MainEngine()
{
    sAppName = "EInk Display";
}

bool MainEngine::OnUserCreate()
{
    return true;
}

bool MainEngine::OnUserUpdate(float fElapsedTime)
{
    // called once per frame
    for (int x = 0; x < ScreenWidth(); x++) {
        for (int y = 0; y < ScreenHeight(); y++) {
            unsigned char level = rand() % 255;
            Draw(x, y, olc::Pixel(level, level, level));
        }
    }

    // Keep the supervisor happy.
    return true;
}
