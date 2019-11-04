#include "MainEngine.h"

MainEngine::MainEngine()
{
    sAppName = "Example";
}

bool MainEngine::OnUserCreate()
{
    return true;
}

bool MainEngine::OnUserUpdate(float fElapsedTime)
{
    // called once per frame
    for (int x = 0; x < ScreenWidth(); x++)
        for (int y = 0; y < ScreenHeight(); y++)
            Draw(x, y, olc::Pixel(rand() % 255, rand() % 255, rand()% 255));
    return true;
}
