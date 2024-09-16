#include <Arduino.h>
//#include <FS.h>
#include <SD.h>
#include <SPIFFS.h>
#include "SDUtil.h"
#include <M5Unified.h>
#include <nvs.h>
#include <Avatar.h>
#include "StackchanExConfig.h" 
#include "Robot.h"
#include "mod/ModManager.h"
#include "mod/ModBase.h"
#include "mod/AiStackChan/AiStackChanMod.h"
#include "mod/StatusMonitor/StatusMonitorMod.h"
#include "mod/FunctionCallingInfo/FunctionCallingInfoMod.h"

#include "driver/PlayMP3.h"   //lipSync

#include <ServoEasing.hpp> // https://github.com/ArminJo/ServoEasing       
//#include "tts/WebVoiceVoxTTS.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "SpiRamJsonDocument.h"
//#include <ESP32WebServer.h>
//#include <ESPmDNS.h>
#include "MailClient.h"
#include <ESP8266FtpServer.h>

//#include <deque>

#if defined(ENABLE_WAKEWORD)
#include "WakeWord.h"
#include "WakeWordIndex.h"
#endif

#include "chat/ChatGPT/ChatGPT.h"
#include "chat/ChatGPT/FunctionCall.h"
#include "chat/ChatGPT/ChatHistory.h"

#include "WebAPI.h"

#if defined( ENABLE_HEX_LED )
#include "driver/HexLED.h"
#endif

#if defined( ENABLE_CAMERA )
#include "driver/Camera.h"
#endif    //ENABLE_CAMERA

#include "Scheduler.h"
#include "MySchedule.h"
#include "driver/WatchDog.h"
#include "SDUpdater.h"

#define USE_SDCARD
#define WIFI_SSID "SET YOUR WIFI SSID"
#define WIFI_PASS "SET YOUR WIFI PASS"
#define OPENAI_APIKEY "SET YOUR OPENAI APIKEY"
#define VOICEVOX_APIKEY "SET YOUR VOICEVOX APIKEY"
#define STT_APIKEY "SET YOUR STT APIKEY"


StackchanExConfig system_config;
Robot* robot;
ModBase* current_mod;

#define USE_SERVO
#ifdef USE_SERVO
int SERVO_PIN_X;
int SERVO_PIN_Y;
#endif  // USE_SERVO

// NTP接続情報　NTP connection information.
const char* NTPSRV      = "ntp.jst.mfeed.ad.jp";    // NTPサーバーアドレス NTP server address.
const long  GMT_OFFSET  = 9 * 3600;                 // GMT-TOKYO(時差９時間）9 hours time difference.
const int   DAYLIGHT_OFFSET = 0;                    // サマータイム設定なし No daylight saving time setting

/// 関数プロトタイプ宣言 /// 
//void exec_chatGPT(String text);
void check_heap_free_size(void);
void check_heap_largest_free_block(void);

//bool servo_home = false;
bool servo_home = true;

using namespace m5avatar;
Avatar avatar;
const Expression expressions_table[] = {
  Expression::Neutral,
  Expression::Happy,
  Expression::Sleepy,
  Expression::Doubt,
  Expression::Sad,
  Expression::Angry
};



FtpServer ftpSrv;   //set #define FTP_DEBUG in ESP8266FtpServer.h to see ftp verbose on serial

//---------------------------------------------
String STT_API_KEY = "";


//---------------------------------------------
#if defined(ENABLE_WAKEWORD)
bool wakeword_is_enable = false;
#endif
//---------------------------------------------



// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  (void) isUnicode; // Punt this ball for now
  // Note that the type and string may be in PROGMEM, so copy them to RAM for printf
  char s1[32], s2[64];
  strncpy_P(s1, type, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  strncpy_P(s2, string, sizeof(s2));
  s2[sizeof(s2)-1]=0;
  Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);
  Serial.flush();
}

// Called when there's a warning or error (like a buffer underflow or decode hiccup)
void StatusCallback(void *cbData, int code, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  // Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}

#ifdef USE_SERVO
#define START_DEGREE_VALUE_X 90
//#define START_DEGREE_VALUE_Y 90
#define START_DEGREE_VALUE_Y 85 //
ServoEasing servo_x;
ServoEasing servo_y;
#endif

void lipSync(void *args)
{
  float gazeX, gazeY;
  int level = 0;
  DriveContext *ctx = (DriveContext *)args;
  Avatar *avatar = ctx->getAvatar();
  for (;;)
  {
    level = abs(*out.getBuffer());
    //level = abs(*(out->getBuffer()));
    if(level<100) level = 0;
    if(level > 15000)
    {
      level = 15000;
    }
    float open = (float)level/15000.0;
    avatar->setMouthOpenRatio(open);
    avatar->getGaze(&gazeY, &gazeX);
    avatar->setRotation(gazeX * 5);
    delay(50);
  }
}

void servo(void *args)
{
  float gazeX, gazeY;
  DriveContext *ctx = (DriveContext *)args;
  Avatar *avatar = ctx->getAvatar();
  for (;;)
  {
#ifdef USE_SERVO
    if(!servo_home)
    {
    avatar->getGaze(&gazeY, &gazeX);
    servo_x.setEaseTo(START_DEGREE_VALUE_X + (int)(15.0 * gazeX));
    if(gazeY < 0) {
      int tmp = (int)(10.0 * gazeY);
      if(tmp > 10) tmp = 10;
      servo_y.setEaseTo(START_DEGREE_VALUE_Y + tmp);
    } else {
      servo_y.setEaseTo(START_DEGREE_VALUE_Y + (int)(10.0 * gazeY));
    }
    } else {
//     avatar->setRotation(gazeX * 5);
//     float b = avatar->getBreath();
       servo_x.setEaseTo(START_DEGREE_VALUE_X); 
//     servo_y.setEaseTo(START_DEGREE_VALUE_Y + b * 5);
       servo_y.setEaseTo(START_DEGREE_VALUE_Y);
    }
    synchronizeAllServosStartAndWaitForAllServosToStop();
#endif
    delay(50);
  }
}

void Servo_setup() {
#ifdef USE_SERVO
  if (!servo_x.attach(SERVO_PIN_X, START_DEGREE_VALUE_X, DEFAULT_MICROSECONDS_FOR_0_DEGREE, DEFAULT_MICROSECONDS_FOR_180_DEGREE)) {
    Serial.print("Error attaching servo x");
  }
  if (!servo_y.attach(SERVO_PIN_Y, START_DEGREE_VALUE_Y, DEFAULT_MICROSECONDS_FOR_0_DEGREE, DEFAULT_MICROSECONDS_FOR_180_DEGREE)) {
    Serial.print("Error attaching servo y");
  }
  servo_x.setEasingType(EASE_QUADRATIC_IN_OUT);
  servo_y.setEasingType(EASE_QUADRATIC_IN_OUT);
  setSpeedForAllServos(30);

  servo_x.setEaseTo(START_DEGREE_VALUE_X); 
  servo_y.setEaseTo(START_DEGREE_VALUE_Y);
  synchronizeAllServosStartAndWaitForAllServosToStop();
#endif
}


void Wifi_setup() {
  // 前回接続時情報で接続する
  while (WiFi.status() != WL_CONNECTED) {
    M5.Display.print(".");
    Serial.print(".");
    delay(500);
    // 10秒以上接続できなかったら抜ける
    if ( 10000 < millis() ) {
      break;
    }
  }
  M5.Display.println("");
  Serial.println("");
  // 未接続の場合にはSmartConfig待受
  if ( WiFi.status() != WL_CONNECTED ) {
    WiFi.mode(WIFI_STA);
    WiFi.beginSmartConfig();
    M5.Display.println("Waiting for SmartConfig");
    Serial.println("Waiting for SmartConfig");
    while (!WiFi.smartConfigDone()) {
      delay(500);
      M5.Display.print("#");
      Serial.print("#");
      // 30秒以上接続できなかったら抜ける
      if ( 30000 < millis() ) {
        Serial.println("");
        Serial.println("Reset");
        ESP.restart();
      }
    }
    // Wi-fi接続
    M5.Display.println("");
    Serial.println("");
    M5.Display.println("Waiting for WiFi");
    Serial.println("Waiting for WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      M5.Display.print(".");
      Serial.print(".");
      // 60秒以上接続できなかったら抜ける
      if ( 60000 < millis() ) {
        Serial.println("");
        Serial.println("Reset");
        ESP.restart();
      }
    }
  }
}

void time_sync(const char* ntpsrv, long gmt_offset, int daylight_offset) {
  struct tm timeInfo; 
  char buf[60];

  configTime(gmt_offset, daylight_offset, ntpsrv);          // NTPサーバと同期

  if (getLocalTime(&timeInfo)) {                            // timeinfoに現在時刻を格納
    Serial.print("NTP : ");                                 // シリアルモニターに表示
    Serial.println(ntpsrv);                                 // シリアルモニターに表示

    sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d\n",     // 表示内容の編集
    timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
    timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);

    Serial.println(buf);                                    // シリアルモニターに表示
  }
  else {
    Serial.print("NTP Sync Error ");                        // シリアルモニターに表示
  }
}



ModBase* init_mod(void)
{
  ModBase* mod;
  add_mod(new AiStackChanMod());
  add_mod(new StatusMonitorMod());
  add_mod(new FunctionCallingInfoMod());
  mod = get_current_mod();
  mod->init();
  return mod;
}

void setup()
{
  auto cfg = M5.config();

  cfg.external_spk = true;    /// use external speaker (SPK HAT / ATOMIC SPK)
//cfg.external_spk_detail.omit_atomic_spk = true; // exclude ATOMIC SPK
//cfg.external_spk_detail.omit_spk_hat    = true; // exclude SPK HAT
//  cfg.output_power = true;
  M5.begin(cfg);

  /// シリアル出力のログレベルを VERBOSEに設定
  M5.Log.setLogLevel(m5::log_target_serial, ESP_LOG_VERBOSE);

#if defined(ENABLE_SD_UPDATER)
  // ***** for SD-Updater *********************
  SDU_lobby("AiStackChan2FuncCall");
  // ******************************************
#endif

  //auto brightness = M5.Display.getBrightness();
  //Serial.printf("Brightness: %d\n", brightness);

  chat_doc = SpiRamJsonDocument(1024*50);

  {
    auto micConfig = M5.Mic.config();
    micConfig.stereo = false;
    micConfig.sample_rate = 16000;
    M5.Mic.config(micConfig);
  }
  M5.Mic.begin();

  { /// custom setting
    auto spk_cfg = M5.Speaker.config();
    /// Increasing the sample_rate will improve the sound quality instead of increasing the CPU load.
    spk_cfg.sample_rate = 96000; // default:64000 (64kHz)  e.g. 48000 , 50000 , 80000 , 96000 , 100000 , 128000 , 144000 , 192000 , 200000
    spk_cfg.task_pinned_core = APP_CPU_NUM;
    //spk_cfg.dma_buf_count = 8;
    //spk_cfg.dma_buf_len = 128;
    M5.Speaker.config(spk_cfg);
  }
  //M5.Speaker.begin();

  M5.Lcd.setTextSize(2);
  Serial.println("Connecting to WiFi");
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
#ifndef USE_SDCARD
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  OPENAI_API_KEY = String(OPENAI_APIKEY);
  VOICEVOX_API_KEY = String(VOICEVOX_APIKEY);
  STT_API_KEY = String(STT_APIKEY);
#else

  /// settings
  if (SD.begin(GPIO_NUM_4, SPI, 25000000)) {

    // この関数ですべてのYAMLファイル(Basic, Secret, Extend)を読み込む
    system_config.loadConfig(SD, "/app/AiStackChan2FuncCall/SC_ExConfig.yaml");

    robot = new Robot(system_config);
    // TODO 以降の設定をRobotに集約していく

#ifdef USE_SERVO
    // Servo
    SERVO_PIN_X = system_config.getServoInfo(AXIS_X)->pin;
    SERVO_PIN_Y = system_config.getServoInfo(AXIS_Y)->pin;
    Serial.printf("Servo pin X:%d, Y:%d\n", SERVO_PIN_X, SERVO_PIN_Y);
    Servo_setup();
    delay(1000);
#endif

    // Wifi
    wifi_s* wifi_info = system_config.getWiFiSetting();
    Serial.printf("SSID: %s\n",wifi_info->ssid.c_str());
    Serial.printf("Key: %s\n",wifi_info->password.c_str());
    WiFi.begin(wifi_info->ssid.c_str(), wifi_info->password.c_str());

    /// API key
    api_keys_s* api_key = system_config.getAPISetting();
    OPENAI_API_KEY = api_key->ai_service;
    //VOICEVOX_API_KEY = api_key->tts;
    STT_API_KEY = api_key->stt;

    // Function Call関連の設定
    init_func_call_settings(&system_config);

    //SD.end();
  } else {
    M5.Lcd.print("Failed to load SD card settings");
    WiFi.begin();
  }
  
#endif  //USE_SDCARD

  M5.Lcd.print("Connecting");
  Wifi_setup();
  M5.Lcd.println("\nConnected");
  Serial.printf_P(PSTR("Go to http://"));
  M5.Lcd.print("Go to http://");
  Serial.println(WiFi.localIP());
  M5.Lcd.println(WiFi.localIP());

  //if (MDNS.begin("m5stack")) {
  //  Serial.println("MDNS responder started");
  //  M5.Lcd.println("MDNS responder started");
  //}
  delay(1000);

  init_web_server();

  init_chat_doc(json_ChatString.c_str());
  // SPIFFSをマウントする
  if(SPIFFS.begin(true)){
    // JSONファイルを開く
    File file = SPIFFS.open("/data.json", "r");
    if(file){
      DeserializationError error = deserializeJson(chat_doc, file);
      if(error){
        Serial.println("Failed to deserialize JSON. Init doc by default.");
        init_chat_doc(json_ChatString.c_str());
      }
      else{
        //const char* role = chat_doc["messages"][1]["content"];
        String role = String((const char*)chat_doc["messages"][1]["content"]);
        
        Serial.println(role);

        if (role != "") {
          init_chat_doc(json_ChatString.c_str());
          JsonArray messages = chat_doc["messages"];
          JsonObject systemMessage1 = messages.createNestedObject();
          systemMessage1["role"] = "system";
          systemMessage1["content"] = role;
          //serializeJson(chat_doc, InitBuffer);
        } else {
          init_chat_doc(json_ChatString.c_str());
        }
      }
      serializeJson(chat_doc, InitBuffer);
      //Role_JSON = InitBuffer;
      String json_str; 
      serializeJsonPretty(chat_doc, json_str);  // 文字列をシリアルポートに出力する
      Serial.println(json_str);
    } else {
      Serial.println("Failed to open file for reading");
      init_chat_doc(json_ChatString.c_str());
    }

  } else {
    Serial.println("An Error has occurred while mounting SPIFFS");
  }

  //SDカードのMP3ファイル（アラーム用）をSPIFFSにコピーする（SDカードだと音が途切れ途切れになるため）。
  //すでにSPIFFSにファイルがあればコピーはしない。強制的にコピー（上書き）したい場合は第2引数をtrueにする。
  copySDFileToSPIFFS("/alarm.mp3", false);

  ftpSrv.begin("stackchan","stackchan");    //username, password for ftp.  set ports in ESP8266FtpServer.h  (default 21, 50009 for PASV)
  Serial.println("FTP server started");
  M5.Lcd.println("FTP server started");  


  Serial.printf_P(PSTR("/ to control the chatGpt Server.\n"));
  M5.Lcd.print("/ to control the chatGpt Server.\n");
  delay(3000);

  audioLogger = &Serial;
  mp3_init();

#if defined(ENABLE_WAKEWORD)
  wakeword_init();
#endif

#if defined(ENABLE_CAMERA)
  avatar.init(16);
#else
  avatar.init();
#endif
  avatar.addTask(lipSync, "lipSync");
  avatar.addTask(servo, "servo");
  avatar.setSpeechFont(&fonts::efontJA_16);

  M5.Speaker.setVolume(180);

  //時刻同期
  time_sync(NTPSRV, GMT_OFFSET, DAYLIGHT_OFFSET);
  
  //メール受信設定
  imap_init();

#if defined( ENABLE_HEX_LED )
  hex_led_init();
  hex_led_ptn_off();
  //hex_led_ptn_boot();
#endif 

#if defined(ENABLE_CAMERA)
  camera_init();
  avatar.set_isSubWindowEnable(true);
#endif

  init_schedule();
  //init_watchdog();

  //mod設定
  current_mod = init_mod();


  //ヒープメモリ残量確認(デバッグ用)
  check_heap_free_size();
  check_heap_largest_free_block();

}


void sw_tone(){
#if 1
  M5.Mic.end();
    M5.Speaker.tone(1000, 100);
  delay(500);
    M5.Speaker.end();
  M5.Mic.begin();
#endif
}

void loop()
{

  M5.update();
  current_mod = get_current_mod();

  if (M5.BtnA.wasPressed())
  {
    current_mod->btnA_pressed();
  }

  if (M5.BtnB.pressedFor(2000))
  {
    current_mod->btnB_longPressed();
  }

  if (M5.BtnC.wasPressed())
  {
    current_mod->btnC_pressed();
  }


#if defined(ARDUINO_M5STACK_Core2) || defined( ARDUINO_M5STACK_CORES3 )
  auto count = M5.Touch.getCount();
  if (count)
  {
    auto t = M5.Touch.getDetail();
    if (t.wasPressed())
    {
      current_mod->display_touched(t.x, t.y);
    }
  }
#endif


  web_server_handle_client();
  ftpSrv.handleFTP();


#if defined( ENABLE_HEX_LED )
  if(recvMessages.size() > 0){
    hex_led_ptn_notification();
  }
#endif

  run_schedule();
  //reset_watchdog();

  current_mod->idle();
  
}


void check_heap_free_size(void){
  Serial.printf("===============================================================\n");
  Serial.printf("Check free heap size\n");
  Serial.printf("===============================================================\n");
  //Serial.printf("esp_get_free_heap_size()                              : %6d\n", esp_get_free_heap_size() );
  Serial.printf("heap_caps_get_free_size(MALLOC_CAP_DMA)               : %6d\n", heap_caps_get_free_size(MALLOC_CAP_DMA) );
  Serial.printf("heap_caps_get_free_size(MALLOC_CAP_SPIRAM)            : %6d\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM) );
  Serial.printf("heap_caps_get_free_size(MALLOC_CAP_INTERNAL)          : %6d\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL) );
  Serial.printf("heap_caps_get_free_size(MALLOC_CAP_DEFAULT)           : %6d\n", heap_caps_get_free_size(MALLOC_CAP_DEFAULT) );

}

void check_heap_largest_free_block(void){
  Serial.printf("===============================================================\n");
  Serial.printf("Check largest free heap block\n");
  Serial.printf("===============================================================\n");
  Serial.printf("heap_caps_get_largest_free_block(MALLOC_CAP_DMA)      : %6d\n", heap_caps_get_largest_free_block(MALLOC_CAP_DMA) );
  Serial.printf("heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)   : %6d\n", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) );
  Serial.printf("heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) : %6d\n", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) );
  Serial.printf("heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)  : %6d\n", heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT) );

}
