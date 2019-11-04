#include "MainEngine.h"

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

int main()
{
    MainEngine demo;
    if (demo.Construct(256, 240, 4, 4))
        demo.Start();

    return 0;
}
