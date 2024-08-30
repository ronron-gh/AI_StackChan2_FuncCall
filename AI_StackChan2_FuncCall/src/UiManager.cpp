#include <Arduino.h>
#include <deque>
#include <SPIFFS.h>
#include "UiManager.h"
#include <Avatar.h>
#include "WakeWord.h"
#include "ChatGPT.h"
#include "Speech.h"
#include "PlayMP3.h"
#include <WiFiClientSecure.h>
#include "Scheduler.h"
#include "FunctionCall.h"
#if defined( ENABLE_CAMERA )
#include <Camera.h>
#endif

using namespace m5avatar;


/// 外部参照 ///
extern Avatar avatar;
extern bool wakeword_is_enable;
extern int random_time;
extern bool random_speak;
extern void sw_tone();
extern void report_batt_level();
extern void STT_ChatGPT(const char *base64_buf = nullptr);
extern void switch_monologue_mode();

extern String random_words[];
extern int random_time;
extern bool random_speak;
extern int lastms1;

///////////////


std::deque<UiBase*> uiList;
//UiBase* uiList[MAX_UI_NUM] = {nullptr};
//UiBase *current_ui;


void add_ui(UiBase* ui)
{
  uiList.push_back(ui);
}

UiBase* change_ui(void)
{
  UiBase* ui;
  ui = uiList[0];
  uiList.pop_front();
  uiList.push_back(ui);
  ui = uiList[0];
  ui->init();
  return ui;
}

UiBase* get_current_ui(void)
{
  return uiList[0];
}

UiBase* init_ui(void)
{
  UiBase* ui;
  add_ui(new UiAvatar());
  add_ui(new UiStatus());
  add_ui(new UiFuncCallInfo());
  ui = get_current_ui();
  ui->init();
  return ui;
}




static bool g_avatar_status = true;
static void avatar_stop()
{
  if(!g_avatar_status)
    return;

  avatar.suspend();
  g_avatar_status = false;
  delay(100);
  Serial.println("avatar suspended");
}

static void avatar_resume()
{
  if(g_avatar_status)
    return;

  avatar.resume();
  Serial.println("avatar resumed");
  g_avatar_status = true;
  delay(100);
}

static void init_info_disp()
{
  //M5.Display.setTextFont(0);
  M5.Display.setFont(&fonts::lgfxJapanGothic_20);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(WHITE, BLACK);
  M5.Display.setTextDatum(0);
  M5.Display.setCursor(0, 0);
  M5.Display.fillScreen(BLACK);
  
  delay(100);
}


UiBase::UiBase(void){};
void UiBase::init(void){};
void UiBase::btnA_pressed(void){};
void UiBase::btnA_longPressed(void){};
void UiBase::btnB_pressed(void){};
void UiBase::btnB_longPressed(void){};
void UiBase::btnC_pressed(void){};
void UiBase::btnC_longPressed(void){};
void UiBase::display_touched(int16_t x, int16_t y){};
void UiBase::idle(void){};

UiAvatar::UiAvatar(void)
{
  box_servo.setupBox(80, 120, 80, 80);
#if defined(ENABLE_CAMERA)
  box_stt.setupBox(107, 0, M5.Display.width()-107, 80);
  box_subWindow.setupBox(0, 0, 107, 80);
#else
  box_stt.setupBox(0, 0, M5.Display.width(), 60);
#endif
  box_BtnA.setupBox(0, 100, 40, 60);
  box_BtnC.setupBox(280, 100, 40, 60);
}

void UiAvatar::init(void)
{
  avatar_resume();
}


void UiAvatar::update(int page_no)
{

}

void UiAvatar::btnA_pressed(void)
{
#if defined(ENABLE_WAKEWORD)
  if(mode >= 0){
    sw_tone();
    if(mode == 0){
      avatar.setSpeechText("ウェイクワード有効");
      mode = 1;
      wakeword_is_enable = true;
    } else {
      avatar.setSpeechText("ウェイクワード無効");
      mode = 0;
      wakeword_is_enable = false;
    }
    delay(1000);
    avatar.setSpeechText("");
  }
#endif
}


void UiAvatar::btnB_longPressed(void)
{
#if defined(ENABLE_WAKEWORD)
  M5.Mic.end();
  M5.Speaker.tone(1000, 100);
  delay(500);
  M5.Speaker.tone(600, 100);
  delay(1000);
  M5.Speaker.end();
  M5.Mic.begin();
  random_time = -1;           //独り言停止
  random_speak = true;
  wakeword_is_enable = false; //wakeword 無効
  mode = -1;
#ifdef USE_SERVO
    servo_home = true;
    delay(500);
#endif
  avatar.setSpeechText("ウェイクワード登録開始");
#endif
}

void UiAvatar::btnC_pressed(void)
{
  sw_tone();
  report_batt_level();
}

void UiAvatar::display_touched(int16_t x, int16_t y)
{
  if (box_stt.contain(x, y)/*&&(!mp3->isRunning())*/)
  {
    sw_tone();
#if defined(ENABLE_CAMERA)
    avatar.set_isSubWindowEnable(false);
    if(isSubWindowON){
      String base64;
      bool ret = camera_capture_base64(base64);
      STT_ChatGPT(base64.c_str());
    }
    else{
      STT_ChatGPT();
    }
    avatar.set_isSubWindowEnable(isSubWindowON);
#else
    STT_ChatGPT();
#endif
  }
#ifdef USE_SERVO
  if (box_servo.contain(t.x, t.y))
  {
    //servo_home = !servo_home;
    sw_tone();
  }
#endif
  if (box_BtnA.contain(x, y))
  {
#if defined(ENABLE_CAMERA)
    isSilentMode = !isSilentMode;
    if(isSilentMode){
      avatar.setSpeechText("サイレントモード");
    }
    else{
      avatar.setSpeechText("サイレントモード解除");
    }
    delay(2000);
    avatar.setSpeechText("");
#else
    sw_tone();
    switch_monologue_mode();
#endif
  }
  if (box_BtnC.contain(x, y))
  {
    //sw_tone();
    change_ui();
  }
#if defined(ENABLE_CAMERA)
  if (box_subWindow.contain(x, y))
  {
    isSubWindowON = !isSubWindowON;
    avatar.set_isSubWindowEnable(isSubWindowON);
  }
#endif //ENABLE_CAMERA

}

void UiAvatar::idle(void)
{

  /// Face detect ///
#if defined(ENABLE_CAMERA)
  //顔が検出されれば音声認識を開始。
  bool isFaceDetected;
  isFaceDetected = camera_capture_and_face_detect();
  if(!isSilentMode){

#if defined(ENABLE_FACE_DETECT)
    if(isFaceDetected){
      avatar.set_isSubWindowEnable(false);
      sw_tone();
      STT_ChatGPT();                              //音声認識
      //exec_chatGPT(random_words[random(18)]);   //独り言

      // フレームバッファを読み捨てる（ｽﾀｯｸﾁｬﾝが応答した後に、過去のフレームで顔検出してしまうのを防ぐため）
      M5.In_I2C.release();
      camera_fb_t *fb = esp_camera_fb_get();
      esp_camera_fb_return(fb);
      avatar.set_isSubWindowEnable(isSubWindowON);
    }
#endif
  }
  else{
#if defined(ENABLE_FACE_DETECT)
    if(isFaceDetected){
      avatar.setExpression(Expression::Happy);
      //delay(2000);
      //avatar.setExpression(Expression::Neutral);
    }
    else{
      avatar.setExpression(Expression::Neutral);
    }
#endif
  }

#endif  //ENABLE_CAMERA


  /// 独り言 ///
  static int lastms = 0;

  if (random_time >= 0 && millis() - lastms1 > random_time)
  {
    lastms1 = millis();
    random_time = 40000 + 1000 * random(30);

    exec_chatGPT(random_words[random(18)]);
#if defined(ENABLE_WAKEWORD)
    mode = 0;
#endif    
  }

  /// Wakeword ///
#if defined(ENABLE_WAKEWORD)
  if (mode == 0) { /* return; */ }
  else if (mode < 0) {
    int idx = wakeword_regist();
    if(idx >= 0){
      String text = String("ウェイクワード#") + String(idx) + String("登録終了");
      avatar.setSpeechText(text.c_str());
      delay(1000);
      avatar.setSpeechText("");
      //mode = 0;
      //wakeword_is_enable = false;
      mode = 1;
      wakeword_is_enable = true;
#ifdef USE_SERVO
      //servo_home = false;
#endif
    }
  }
  else if (mode > 0 && wakeword_is_enable) {
    int idx = wakeword_compare();
    if( idx >= 0){
      Serial.println("wakeword_compare OK!");
      String text = String("ウェイクワード#") + String(idx);
      avatar.setSpeechText(text.c_str());
      sw_tone();
      STT_ChatGPT();
    }
  }

  // Function Callからの要求でウェイクワード有効化
  if (wakeword_enable_required)
  {
    wakeword_enable_required = false;
    btnA_pressed();
  }

  // Function Callからの要求でウェイクワード登録
  if(register_wakeword_required)
  {
    register_wakeword_required = false;
    
  }
#endif  //ENABLE_WAKEWORD

  /// Alarm ///
  if(xAlarmTimer != NULL){
    TickType_t xRemainingTime;

    /* Query the period of the timer that expires. */
    xRemainingTime = xTimerGetExpiryTime( xAlarmTimer ) - xTaskGetTickCount();
    avatarText = "残り" + String(xRemainingTime / 1000) + "秒";
    avatar.setSpeechText(avatarText.c_str());
  }

  if (alarmTimerCallbacked) {
    alarmTimerCallbacked = false;
#if defined(ENABLE_CAMERA)
    avatar.set_isSubWindowEnable(false);
#endif    
    //if(!SD.begin(GPIO_NUM_4, SPI, 25000000)) {
    if(!SPIFFS.begin(true)){
      Serial.println("Failed to mount SPIFFS.");
    }
    else{
      playMP3File("/alarm.mp3");
      speech("時間になりました。");
    }
    avatar.set_isSubWindowEnable(isSubWindowON);
  }

}

UiStatus::UiStatus(void)
{
  box_BtnA.setupBox(0, 100, 40, 60);
  box_BtnC.setupBox(280, 100, 40, 60);
}

void UiStatus::init(void)
{
  avatar_stop();
  update(0);
}

void UiStatus::update(int page_no)
{
  String str = "";
  char tmp[256];

  init_info_disp();

  str += "======== System status ========\n";
  sprintf(tmp, "Wifi:\n  IP addr:%s\n",
                  WiFi.localIP().toString().c_str() );
  str += tmp;
  sprintf(tmp, "Heap largest free block:\n  DMA:%d\n  SPIRAM:%d\n",
                  heap_caps_get_largest_free_block(MALLOC_CAP_DMA),
                  heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) );
  str += tmp;
  sprintf(tmp, "Prompt buffer usage:\n  %d / %d [byte]\n",
                  chat_doc.memoryUsage(),
                  chat_doc.capacity() );
  str += tmp;
  sprintf(tmp, "Battery level:  %d %%\n", M5.Power.getBatteryLevel());
  str += tmp;
  M5.Display.print(str);
}

void UiStatus::display_touched(int16_t x, int16_t y)
{

  if (box_BtnA.contain(x, y))
  {

  }

  if (box_BtnC.contain(x, y))
  {
    //sw_tone();
    change_ui();
  }


}




UiFuncCallInfo::UiFuncCallInfo(void)
{
  box_BtnA.setupBox(0, 100, 40, 60);
  box_BtnC.setupBox(280, 100, 40, 60);
  box_BtnUA.setupBox(0, 0, 80, 60);
  box_BtnUC.setupBox(240, 0, 80, 60);

  current_page_no = 0;
}

void UiFuncCallInfo::init(void)
{
  avatar_stop();
  update(current_page_no);
}

void UiFuncCallInfo::update(int page_no)
{
  String str = "";
  char tmp[256];

  init_info_disp();

  str += "<- prev                 next ->\n";
  str += "===== Function call info  =====\n";
  if(page_no == 0){
    str += "Reminder:\n";
    for(int i=0; i<MAX_SCHED_NUM; i++){
      ScheduleBase *sched = get_schedule(i);
      if(sched != nullptr){
          if (sched->get_sched_type() == SCHED_REMINDER) {  
              ScheduleReminder *sched_rem = (ScheduleReminder*)sched;
              str += " " + sched_rem->get_time_string() + " " + sched_rem->get_remind_string() + "\n";
          }
      }
    }
  }
  else if(page_no == 1){
    str += "Memo:\n";
    str += note;
  }

  M5.Display.print(str);

}

void UiFuncCallInfo::display_touched(int16_t x, int16_t y)
{

  if (box_BtnA.contain(x, y))
  {

  }

  if (box_BtnC.contain(x, y))
  {
    //sw_tone();
    change_ui();
  }

  if (box_BtnUA.contain(x, y))
  {
    current_page_no--;
    if(current_page_no < 0) current_page_no = MAX_PAGE_NO - 1;
    update(current_page_no);
  }

  if (box_BtnUC.contain(x, y))
  {
    current_page_no++;
    if(current_page_no >= MAX_PAGE_NO) current_page_no = 0;
    update(current_page_no);
  }

}