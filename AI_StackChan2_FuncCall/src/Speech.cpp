#include <Arduino.h>
#include <M5Unified.h>
#include "Speech.h"
#include "WebVoiceVoxTTS.h"
#include "PlayMP3.h"
#include "Avatar.h"

using namespace m5avatar;

extern Avatar avatar;
extern bool servo_home;

void speech(String text)
{
    servo_home = false;
    avatar.setExpression(Expression::Happy);

    M5.Mic.end();
    M5.Speaker.begin();

    Voicevox_tts((char*)text.c_str(), (char*)TTS_PARMS.c_str());

    while(mp3->isRunning()) {
        if (!mp3->loop()) {
            mp3->stop();
            Serial.println("mp3 stop");
            avatar.setExpression(Expression::Neutral);
            delay(200);
            M5.Speaker.end();
            M5.Mic.begin();
            servo_home = true;
        }
        delay(1);
    }
}