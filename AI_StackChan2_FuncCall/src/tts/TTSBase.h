#ifndef _TTS_BASE_H
#define _TTS_BASE_H

#include <Arduino.h>

struct tts_param_t
{
  String api_key;
  String model;
  String voice;

};


class TTSBase{
protected:
    tts_param_t param;
public:
    TTSBase(tts_param_t param) : param{param} {};
    virtual void stream(String text) = 0;

};



#endif //_TTS_BASE_H