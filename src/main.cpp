#include "MainEngine.h"

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

int main()
{
    MainEngine demo;
    if (demo.Construct(WINDOW_WIDTH, WINDOW_HEIGHT, 2, 2))
        demo.Start();

    return 0;
}
