#include <Arduino.h>
#include <deque>
#include <SPIFFS.h>
#include "mod/ModManager.h"
#include "PomodoroMod.h"
#include <Avatar.h>
#include "Robot.h"
#include "WakeWord.h"
#include "driver/PlayMP3.h"
#include <WiFiClientSecure.h>

#include "driver/AudioWhisper.h"       //speechToText
#include "stt/Whisper.h"               //speechToText
#include "driver/Audio.h"              //speechToText
#include "stt/CloudSpeechClient.h"     //speechToText
#include "rootCA/rootCACertificate.h"  //speechToText
#include "rootCA/rootCAgoogle.h"       //speechToText
#include "driver/Audio.h"              //speechToText


using namespace m5avatar;


/// 外部参照 ///
extern Avatar avatar;
extern bool servo_home;
extern bool wakeword_is_enable;
extern void sw_tone();
extern String OPENAI_API_KEY;   // TODO  Robotに隠蔽したい
extern String STT_API_KEY;      // TODO  Robotに隠蔽したい
///////////////

static bool pomodoroTimerCallbacked = false;


void pomodoroTimerCallback(TimerHandle_t _xTimer){
  pomodoroTimerCallbacked = true;

}


PomodoroMod::PomodoroMod()
{
  box_servo.setupBox(80, 120, 80, 80);
  box_stt.setupBox(0, 0, M5.Display.width(), 60);
  box_BtnA.setupBox(0, 100, 40, 60);
  box_BtnC.setupBox(280, 100, 40, 60);

}


void PomodoroMod::init(void)
{
  //avatar_resume();
  xTimer = NULL;
  status = READY;
  avatarText = "Btn A:スタート/リセット";
  avatar.setSpeechFont(&fonts::efontJA_12);
  avatar.setSpeechText(avatarText.c_str());
}

void PomodoroMod::pause(void)
{
  if((status == TIMER25) || (status == TIMER5)){
    xTimerStop(xTimer, 0);
    xTimerDelete(xTimer, 0);
    xTimer = NULL;
    status = READY;
  }
  avatar.setSpeechFont(&fonts::efontJA_16);
  avatar.setSpeechText("");
  //avatar_stop();
}


void PomodoroMod::btnA_pressed(void)
{
  sw_tone();
  if(status == READY){
    xTimer = xTimerCreate("Timer", POMODORO_TIMER_25MIN, pdFALSE, 0, pomodoroTimerCallback);
    if(xTimer != NULL){
      xTimerStart(xTimer, 0);
      status = TIMER25;
      avatar.setSpeechFont(&fonts::efontJA_16);
    }
    else{
      avatar.setExpression(Expression::Sad);
      avatar.setSpeechText("タイマー設定失敗");
      delay(2000);
      avatar.setSpeechText("");
      avatar.setExpression(Expression::Neutral);
    }
  }
  else if((status == TIMER25) || (status == TIMER5)){
    xTimerStop(xTimer, 0);
    xTimerDelete(xTimer, 0);
    init();
  }
}


void PomodoroMod::btnC_pressed(void)
{
}

void PomodoroMod::display_touched(int16_t x, int16_t y)
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

void PomodoroMod::idle(void)
{
  if(xTimer != NULL){
    if(xTimerIsTimerActive( xTimer ) != pdFALSE){
      TickType_t xRemainingTime;
      String phase;
      if(status == TIMER25){
        phase = String("集中 ");
        avatar.setExpression(Expression::Neutral);
      }
      else if(status == TIMER5){
        phase = String("休憩 ");
        avatar.setExpression(Expression::Sleepy);
      }

      xRemainingTime = xTimerGetExpiryTime( xTimer ) - xTaskGetTickCount();
      avatarText = phase + "残り" + String(xRemainingTime / 1000 / 60 + 1) + "分";
      avatar.setSpeechText(avatarText.c_str());
    }

  }


  if (pomodoroTimerCallbacked) {
    pomodoroTimerCallbacked = false;
    if(status == TIMER25){
      robot->speech("集中時間終了です。5分間休憩してください。");
      xTimer = xTimerCreate("Timer", POMODORO_TIMER_5MIN, pdFALSE, 0, pomodoroTimerCallback);
      if(xTimer != NULL){
        xTimerStart(xTimer, 0);
        status = TIMER25;
      }
      else{
        avatar.setExpression(Expression::Sad);
        avatar.setSpeechText("タイマー設定失敗");
        delay(2000);
        avatar.setSpeechText("");
        avatar.setExpression(Expression::Neutral);
      }
      status = TIMER5;
    }
    else if(status == TIMER5){
      robot->speech("休憩時間終了です。");
      xTimer = xTimerCreate("Timer", POMODORO_TIMER_25MIN, pdFALSE, 0, pomodoroTimerCallback);
      if(xTimer != NULL){
        xTimerStart(xTimer, 0);
        status = TIMER25;
      }
      else{
        avatar.setExpression(Expression::Sad);
        avatar.setSpeechText("タイマー設定失敗");
        delay(2000);
        avatar.setSpeechText("");
        avatar.setExpression(Expression::Neutral);
      }
      status = TIMER25;
    }
  }

}

