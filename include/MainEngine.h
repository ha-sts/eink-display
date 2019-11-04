#ifndef MAINENGINE_H
#define MAINENGINE_H

#include "olcPixelGameEngine.h"

#define EINK_WIDTH 640
#define EINK_HEIGHT 385
#define WINDOW_WIDTH EINK_WIDTH+0
#define WINDOW_HEIGHT EINK_HEIGHT+115

class MainEngine : public olc::PixelGameEngine
{
    public:
        MainEngine();
        bool OnUserCreate() override;
        bool OnUserUpdate(float fElapsedTime) override;

    protected:

    private:
};

#endif // MAINENGINE_H
