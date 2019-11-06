#include "MainEngine.h"

MainEngine::MainEngine()
{
    sAppName = "EInk Display";
}

bool MainEngine::OnUserCreate()
{
    //mqttHdl = new MqttWrapper("olc", "127.0.0.1", 1883);

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

    // Keep the MQTT Client happy
    //if(mqttHdl->loop()) {
        // Something is wrong, so reconnect
        //mqttHdl->reconnect();
    //}

    // Keep the supervisor happy.
    return true;
}
