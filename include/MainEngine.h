#ifndef MAINENGINE_H
#define MAINENGINE_H

#include "olcPixelGameEngine.h"

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
