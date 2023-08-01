#ifndef _FUNCTION_CALL_H
#define _FUNCTION_CALL_H

#include <Arduino.h>
#include <ArduinoJson.h>

extern String json_ChatString;
extern String weatherCityID;
extern String note;

String exec_calledFunc(DynamicJsonDocument doc, String* calledFunc);

#endif //_FUNCTION_CALL_H