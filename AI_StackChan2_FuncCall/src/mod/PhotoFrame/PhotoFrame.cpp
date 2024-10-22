#include <Arduino.h>
#include <deque>
#include <SD.h>
#include "SDUtil.h"
#include "mod/ModManager.h"
#include "PhotoFrameMod.h"
#include <Avatar.h>
#include "Robot.h"
#include "WakeWord.h"
#include "driver/PlayMP3.h"
#include <WiFiClientSecure.h>

#include "driver/Camera.h"

#include "chat/ChatGPT/FunctionCall.h"  //APP_DATA_PATHのため

using namespace m5avatar;


/// 外部参照 ///
extern Avatar avatar;
extern bool servo_home;
extern bool wakeword_is_enable;
extern void sw_tone();
extern String OPENAI_API_KEY;   // TODO  Robotに隠蔽したい
extern String STT_API_KEY;      // TODO  Robotに隠蔽したい
///////////////


PhotoFrameMod::PhotoFrameMod(bool _isOffline)
  : isOffline{_isOffline}
{
  box_servo.setupBox(80, 120, 80, 80);
  box_stt.setupBox(0, 0, M5.Display.width(), 60);
  box_BtnA.setupBox(0, 100, 40, 60);
  box_BtnC.setupBox(280, 100, 40, 60);

}


void PhotoFrameMod::init(void)
{

  avatarText = "Photo Frame";
  avatar.setSpeechText(avatarText.c_str());

  uint8_t *jpgBuf = new uint8_t[262144];  //256KB
  String fname = String(APP_DATA_PATH) + "photo/photo001.jpg";
  size_t jpgSize = copySDFileToRAM(fname.c_str(), jpgBuf, 262144);

  //uint8_t *rgb565Buf = new uint8_t[262144];
  //jpg2rgb565((const uint8_t*)jpgBuf, jpgSize, rgb565Buf, JPG_SCALE_2X);



  avatar.updateSubWindowJpg(jpgBuf, jpgSize);
  avatar.set_isSubWindowEnable(true);
  delete[] jpgBuf;
  //delete[] rgb565Buf;

}

void PhotoFrameMod::pause(void)
{

  avatar.setSpeechText("");

}


void PhotoFrameMod::btnA_pressed(void)
{
  sw_tone();

}


void PhotoFrameMod::btnB_pressed(void)
{

}

void PhotoFrameMod::btnC_pressed(void)
{

}

void PhotoFrameMod::display_touched(int16_t x, int16_t y)
{
  if (box_stt.contain(x, y))
  {
    sw_tone();
  }

  if (box_servo.contain(x, y))
  {
    //servo_home = !servo_home;
    sw_tone();
  }

  if (box_BtnA.contain(x, y))
  {
    btnA_pressed();
  }

  if (box_BtnC.contain(x, y))
  {
    btnC_pressed();
  }

}

void PhotoFrameMod::idle(void)
{

}

