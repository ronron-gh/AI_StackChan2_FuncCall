#ifndef _ROBOT_H
#define _ROBOT_H

#include <Arduino.h>
#include "StackchanExConfig.h" 
#include "tts/TTSBase.h"



class Robot{
private:
    TTSBase *tts;
public:
    Robot(StackchanExConfig& config);
    void speech(String text);
    String listen(bool isGoogle);

};

extern Robot* robot;

#endif //_ROBOT_H