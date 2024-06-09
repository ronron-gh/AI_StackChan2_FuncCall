#include <Arduino.h>
//#include <FS.h>
#include <SD.h>
#include <SPIFFS.h>
#include "SDUtil.h"
#include <M5Unified.h>
#include <nvs.h>
#include <Avatar.h>

#include "PlayMP3.h"

#include <ServoEasing.hpp> // https://github.com/ArminJo/ServoEasing       
#include "WebVoiceVoxTTS.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "rootCACertificate.h"
#include "rootCAgoogle.h"
#include <ArduinoJson.h>
#include "SpiRamJsonDocument.h"
//#include <ESP32WebServer.h>
//#include <ESPmDNS.h>
#include "MailClient.h"
#include <ESP8266FtpServer.h>

//#include <deque>
#include "AudioWhisper.h"
#include "Whisper.h"
#include "Audio.h"
#include "CloudSpeechClient.h"
#include "Speech.h"
#if defined(ENABLE_WAKEWORD)
#include "WakeWord.h"
#include "WakeWordIndex.h"
#endif

#include "ChatGPT.h"
#include "FunctionCall.h"
#include "ChatHistory.h"

#include "WebAPI.h"

#if defined( ENABLE_HEX_LED )
#include "HexLED.h"
#endif

#if defined( ENABLE_FACE_DETECT )
#include <Camera.h>
#endif    //ENABLE_FACE_DETECT

#include "Scheduler.h"
#include "MySchedule.h"
#include "WatchDog.h"
#include "SDUpdater.h"

#define USE_SDCARD
#define WIFI_SSID "SET YOUR WIFI SSID"
#define WIFI_PASS "SET YOUR WIFI PASS"
#define OPENAI_APIKEY "SET YOUR OPENAI APIKEY"
#define VOICEVOX_APIKEY "SET YOUR VOICEVOX APIKEY"
#define STT_APIKEY "SET YOUR STT APIKEY"

#define USE_SERVO
#ifdef USE_SERVO
#if defined(ENABLE_SD_UPDATER)
  // SERVO_PINはservo.txtから読み込む
#else
#if defined(ARDUINO_M5STACK_Core2)
  // #define SERVO_PIN_X 13  //Core2 PORT C
  // #define SERVO_PIN_Y 14
  #define SERVO_PIN_X 33  //Core2 PORT A
  #define SERVO_PIN_Y 32
#elif defined( ARDUINO_M5STACK_FIRE )
  #define SERVO_PIN_X 21
  #define SERVO_PIN_Y 22
#elif defined( ARDUINO_M5Stack_Core_ESP32 )
  #define SERVO_PIN_X 21
  #define SERVO_PIN_Y 22
#elif defined( ARDUINO_M5STACK_CORES3 )
  #define SERVO_PIN_X 18  //CoreS3 PORT C
  #define SERVO_PIN_Y 17
#endif
#endif  // ENABLE_SD_UPDATER
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
String avatarText;


FtpServer ftpSrv;   //set #define FTP_DEBUG in ESP8266FtpServer.h to see ftp verbose on serial

//---------------------------------------------
//String VOICEVOX_API_KEY = "";
String STT_API_KEY = "";
//String TTS_SPEAKER_NO = "3";
//String TTS_SPEAKER = "&speaker=";
//String TTS_PARMS = TTS_SPEAKER + TTS_SPEAKER_NO;

//---------------------------------------------
#if defined(ENABLE_WAKEWORD)
bool wakeword_is_enable = false;
#endif
//---------------------------------------------

//String speech_text = "";
//String speech_text_buffer = "";




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

struct box_t
{
  int x;
  int y;
  int w;
  int h;
  int touch_id = -1;

  void setupBox(int x, int y, int w, int h) {
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
  }
  bool contain(int x, int y)
  {
    return this->x <= x && x < (this->x + this->w)
        && this->y <= y && y < (this->y + this->h);
  }
};
static box_t box_servo;
static box_t box_stt;
static box_t box_BtnA;
static box_t box_BtnC;
#if defined(ENABLE_FACE_DETECT)
static box_t box_subWindow;
#endif

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

String SpeechToText(bool isGoogle){
  Serial.println("\r\nRecord start!\r\n");

  String ret = "";
  if( isGoogle) {
    Audio* audio = new Audio();
    audio->Record();  
    Serial.println("Record end\r\n");
    Serial.println("音声認識開始");
    avatar.setSpeechText("わかりました");  
    CloudSpeechClient* cloudSpeechClient = new CloudSpeechClient(root_ca_google, STT_API_KEY.c_str());
    ret = cloudSpeechClient->Transcribe(audio);
    delete cloudSpeechClient;
    delete audio;
  } else {
    AudioWhisper* audio = new AudioWhisper();
    audio->Record();  
    Serial.println("Record end\r\n");
    Serial.println("音声認識開始");
    avatar.setSpeechText("わかりました");  
    Whisper* cloudSpeechClient = new Whisper(root_ca_openai, OPENAI_API_KEY.c_str());
    ret = cloudSpeechClient->Transcribe(audio);
    delete cloudSpeechClient;
    delete audio;
  }
  return ret;
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


void setup()
{
  auto cfg = M5.config();

  cfg.external_spk = true;    /// use external speaker (SPK HAT / ATOMIC SPK)
//cfg.external_spk_detail.omit_atomic_spk = true; // exclude ATOMIC SPK
//cfg.external_spk_detail.omit_spk_hat    = true; // exclude SPK HAT
//  cfg.output_power = true;
  M5.begin(cfg);

#if defined(ENABLE_SD_UPDATER)
  // ***** for SD-Updater *********************
  SDU_lobby("AiStackChan2FuncCall");

  // read from SD: "/servo.txt" file
  if (!servoTxtSDRead())
  {
    Serial.println("cannnot read servo.txt file ...");
    SERVO_PIN_X = 13;
    SERVO_PIN_Y = 14;
  }
  // ******************************************
#endif 

  //auto brightness = M5.Display.getBrightness();
  //Serial.printf("Brightness: %d\n", brightness);

  chat_doc = SpiRamJsonDocument(1024*30);

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

  Servo_setup();
  delay(1000);

  {
    uint32_t nvs_handle;
    if (ESP_OK == nvs_open("setting", NVS_READONLY, &nvs_handle)) {
      size_t volume;
      uint8_t led_onoff;
      uint8_t speaker_no;
      nvs_get_u32(nvs_handle, "volume", &volume);
      if(volume > 255) volume = 255;
      M5.Speaker.setVolume(volume);
      M5.Speaker.setChannelVolume(m5spk_virtual_channel, volume);
      nvs_get_u8(nvs_handle, "led", &led_onoff);
      // if(led_onoff == 1) Use_LED = true;
      // else  Use_LED = false;
      nvs_get_u8(nvs_handle, "speaker", &speaker_no);
      if(speaker_no > 60) speaker_no = 3;
      TTS_SPEAKER_NO = String(speaker_no);
      TTS_PARMS = TTS_SPEAKER + TTS_SPEAKER_NO;
      nvs_close(nvs_handle);
    } else {
      if (ESP_OK == nvs_open("setting", NVS_READWRITE, &nvs_handle)) {
        size_t volume = 180;
        uint8_t led_onoff = 0;
        uint8_t speaker_no = 3;
        nvs_set_u32(nvs_handle, "volume", volume);
        nvs_set_u8(nvs_handle, "led", led_onoff);
        nvs_set_u8(nvs_handle, "speaker", speaker_no);
        nvs_close(nvs_handle);
        M5.Speaker.setVolume(volume);
        M5.Speaker.setChannelVolume(m5spk_virtual_channel, volume);
        // Use_LED = false;
        TTS_SPEAKER_NO = String(speaker_no);
        TTS_PARMS = TTS_SPEAKER + TTS_SPEAKER_NO;
      }
    }
  }

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
    /// wifi
    {
      char buf[128], ssid[128], key[128];
      if(read_sd_file("/wifi.txt", buf, sizeof(buf))){
        read_line_from_buf(buf, ssid);
        Serial.printf("SSID: %s\n",ssid);
        read_line_from_buf(nullptr, key);
        Serial.printf("Key: %s\n",key);
        WiFi.begin(ssid, key);
      }
      else{
        WiFi.begin();
      }
    }

    /// API key
    uint32_t nvs_handle;
    if (ESP_OK == nvs_open("apikey", NVS_READWRITE, &nvs_handle)) {
      char buf[256], key[256];
      if(read_sd_file("/apikey.txt", buf, sizeof(buf))){
        read_line_from_buf(buf, key);
        nvs_set_str(nvs_handle, "openai", key);
        Serial.printf("openai: %s\n",key);

        read_line_from_buf(nullptr, key);
        nvs_set_str(nvs_handle, "voicevox", key);
        Serial.printf("voicevox: %s\n",key);

        read_line_from_buf(nullptr, key);
        nvs_set_str(nvs_handle, "sttapikey", key);
        Serial.printf("stt: %s\n",key);
      }
      nvs_close(nvs_handle);
    }

    /// News API key
    {
      char buf[128], key[128];
      if(read_sd_file("/news_api_key.txt", buf, sizeof(buf))){
        read_line_from_buf(buf, key);
        newsApiKey = String(key);
      }
      Serial.printf("NewsAPI key: %s\n", newsApiKey.c_str());
    }

    /// Weather city ID
    {
      char buf[128], key[128];
      if(read_sd_file("/weather_city_id.txt", buf, sizeof(buf))){
        read_line_from_buf(buf, key);
        weatherCityID = String(key);
      }
      else{
        weatherCityID = "130010";
      }
      Serial.printf("Weather City ID: %s\n", weatherCityID);
    }

    /// Gmail
    {
      char buf[256], key[256];
      if(read_sd_file("/gmail.txt", buf, sizeof(buf))){
        read_line_from_buf(buf, key);
        authMailAdr = String(key);
        Serial.println("My mail addr: " + authMailAdr);

        read_line_from_buf(nullptr, key);
        authAppPass = String(key);
        Serial.println("My mail pass: " + authAppPass);

        read_line_from_buf(nullptr, key);
        toMailAdr = String(key);
        Serial.println("To mail addr: " + toMailAdr);
      }
    }

    /// メモがあるか確認
    {
      char buf[512];
      if(read_sd_file("/notepad.txt", buf, sizeof(buf))){
        note = String(buf);
        Serial.printf("Notepad: %s\n", buf);
      }
    }

    //SD.end();
  } else {
    WiFi.begin();
    weatherCityID = "130010";
  }

  {
    uint32_t nvs_handle;
    if (ESP_OK == nvs_open("apikey", NVS_READONLY, &nvs_handle)) {
      Serial.println("nvs_open");

      size_t length1;
      size_t length2;
      size_t length3;
      if(ESP_OK == nvs_get_str(nvs_handle, "openai", nullptr, &length1) && 
         ESP_OK == nvs_get_str(nvs_handle, "voicevox", nullptr, &length2) && 
         ESP_OK == nvs_get_str(nvs_handle, "sttapikey", nullptr, &length3) && 
        length1 && length2 && length3) {
        Serial.println("nvs_get_str");
        char openai_apikey[length1 + 1];
        char voicevox_apikey[length2 + 1];
        char stt_apikey[length3 + 1];
        if(ESP_OK == nvs_get_str(nvs_handle, "openai", openai_apikey, &length1) && 
           ESP_OK == nvs_get_str(nvs_handle, "voicevox", voicevox_apikey, &length2) &&
           ESP_OK == nvs_get_str(nvs_handle, "sttapikey", stt_apikey, &length3)) {
          OPENAI_API_KEY = String(openai_apikey);
          VOICEVOX_API_KEY = String(voicevox_apikey);
          STT_API_KEY = String(stt_apikey);
          // Serial.println(OPENAI_API_KEY);
          // Serial.println(VOICEVOX_API_KEY);
          // Serial.println(STT_API_KEY);
        }
      }
      nvs_close(nvs_handle);
    }
  }
  
#endif

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

#if defined(ENABLE_FACE_DETECT)
  avatar.init(16);
#else
  avatar.init();
#endif
  avatar.addTask(lipSync, "lipSync");
  avatar.addTask(servo, "servo");
  avatar.setSpeechFont(&fonts::efontJA_16);

//  M5.Speaker.setVolume(200);
  box_servo.setupBox(80, 120, 80, 80);
#if defined(ENABLE_FACE_DETECT)
  box_stt.setupBox(107, 0, M5.Display.width()-107, 80);
  box_subWindow.setupBox(0, 0, 107, 80);
#else
  box_stt.setupBox(0, 0, M5.Display.width(), 60);
#endif
  box_BtnA.setupBox(0, 100, 40, 60);
  box_BtnC.setupBox(280, 100, 40, 60);

  //時刻同期
  time_sync(NTPSRV, GMT_OFFSET, DAYLIGHT_OFFSET);
  
  //メール受信設定
  imap_init();

#if defined( ENABLE_HEX_LED )
  hex_led_init();
  hex_led_ptn_off();
  //hex_led_ptn_boot();
#endif 

#if defined(ENABLE_FACE_DETECT)
  camera_init();
  avatar.set_isSubWindowEnable(true);
#endif

  init_schedule();
  //init_watchdog();

  //ヒープメモリ残量確認(デバッグ用)
  check_heap_free_size();
  check_heap_largest_free_block();

}


String random_words[18] = {"あなたは誰","楽しい","怒った","可愛い","悲しい","眠い","ジョークを言って","泣きたい","怒ったぞ","こんにちは","お疲れ様","詩を書いて","疲れた","お腹空いた","嫌いだ","苦しい","俳句を作って","歌をうたって"};
int random_time = -1;
bool random_speak = true;
static int lastms1 = 0;

void report_batt_level(){
  char buff[100];
  int level = M5.Power.getBatteryLevel();
#if defined(ENABLE_WAKEWORD)
  mode = 0;
#endif
  if(M5.Power.isCharging())
    sprintf(buff,"充電中、バッテリーのレベルは%d％です。",level);
  else
    sprintf(buff,"バッテリーのレベルは%d％です。",level);
  avatar.setExpression(Expression::Happy);
#if defined(ENABLE_WAKEWORD)
  mode = 0; 
#endif
  speech(String(buff));
  delay(1000);
  avatar.setExpression(Expression::Neutral);
}

void switch_monologue_mode(){
    String tmp;
#if defined(ENABLE_WAKEWORD)
    mode = 0;
#endif
    if(random_speak) {
      tmp = "独り言始めます。";
      lastms1 = millis();
      random_time = 40000 + 1000 * random(30);
    } else {
      tmp = "独り言やめます。";
      random_time = -1;
    }
    random_speak = !random_speak;
    avatar.setExpression(Expression::Happy);
#if defined(ENABLE_WAKEWORD)
    mode = 0;
#endif
    speech(tmp);
    delay(1000);
    avatar.setExpression(Expression::Neutral);
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

void SST_ChatGPT() {
  bool prev_servo_home = servo_home;
  random_speak = true;
  random_time = -1;
#ifdef USE_SERVO
  servo_home = true;
#endif

#if defined( ENABLE_HEX_LED )
          hex_led_ptn_wake();
#endif

  avatar.setExpression(Expression::Happy);
  avatar.setSpeechText("御用でしょうか？");
  String ret;
  if(OPENAI_API_KEY != STT_API_KEY){
    Serial.println("Google STT");
    ret = SpeechToText(true);
  } else {
    Serial.println("Whisper STT");
    ret = SpeechToText(false);
  }
#ifdef USE_SERVO
  //servo_home = prev_servo_home;
  servo_home = false;
#endif
  Serial.println("音声認識終了");
  Serial.println("音声認識結果");
  if(ret != "") {
    Serial.println(ret);

#if defined( ENABLE_HEX_LED )
    hex_led_ptn_accept();
#endif
    exec_chatGPT(ret);
    avatar.setSpeechText("");
    avatar.setExpression(Expression::Neutral);
    servo_home = true;
#if defined(ENABLE_WAKEWORD)
    mode = 0;
#endif

  } else {
#if defined( ENABLE_HEX_LED )
    hex_led_ptn_off();
#endif
    Serial.println("音声認識失敗");
    avatar.setExpression(Expression::Sad);
    avatar.setSpeechText("聞き取れませんでした");
    delay(2000);
    avatar.setSpeechText("");
    avatar.setExpression(Expression::Neutral);
    servo_home = true;
  } 
}


void loop()
{
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

  
#if defined(ENABLE_FACE_DETECT)
  //顔が検出されれば音声認識を開始。
  bool isFaceDetected;
  isFaceDetected = camera_capture_and_face_detect();
  if(!isSilentMode){
    if(isFaceDetected){
      avatar.set_isSubWindowEnable(false);
      sw_tone();
      SST_ChatGPT();                              //音声認識
      //exec_chatGPT(random_words[random(18)]);   //独り言

      // フレームバッファを読み捨てる（ｽﾀｯｸﾁｬﾝが応答した後に、過去のフレームで顔検出してしまうのを防ぐため）
      M5.In_I2C.release();
      camera_fb_t *fb = esp_camera_fb_get();
      esp_camera_fb_return(fb);
    }
  }
  else{
    if(isFaceDetected){
      avatar.setExpression(Expression::Happy);
      //delay(2000);
      //avatar.setExpression(Expression::Neutral);
    }
    else{
      avatar.setExpression(Expression::Neutral);
    }
  }

#endif

  M5.update();

#if defined(ENABLE_WAKEWORD)
  if (M5.BtnA.wasPressed() || wakeword_enable_required)
  {
    wakeword_enable_required = false;

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
  }

  if (M5.BtnB.pressedFor(2000) || register_wakeword_required) {
    register_wakeword_required = false;

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
  }
#endif  //ENABLE_WAKEWORD

  if (M5.BtnC.wasPressed())
  {
    sw_tone();
    report_batt_level();
  }


#if defined(ARDUINO_M5STACK_Core2) || defined( ARDUINO_M5STACK_CORES3 )
  auto count = M5.Touch.getCount();
  if (count)
  {
    auto t = M5.Touch.getDetail();
    if (t.wasPressed())
    {          
      if (box_stt.contain(t.x, t.y)&&(!mp3->isRunning()))
      {
#if defined(ENABLE_FACE_DETECT)
        avatar.set_isSubWindowEnable(false);
#endif
        sw_tone();
        SST_ChatGPT();
      }
#ifdef USE_SERVO
      if (box_servo.contain(t.x, t.y))
      {
        //servo_home = !servo_home;
        sw_tone();
      }
#endif
      if (box_BtnA.contain(t.x, t.y))
      {
#if defined(ENABLE_FACE_DETECT)
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
      if (box_BtnC.contain(t.x, t.y))
      {
#if defined(ENABLE_FACE_DETECT)
        avatar.set_isSubWindowEnable(false);
#endif
        sw_tone();
        report_batt_level();
      }
#if defined(ENABLE_FACE_DETECT)
      if (box_subWindow.contain(t.x, t.y))
      {
        isSubWindowON = !isSubWindowON;
        avatar.set_isSubWindowEnable(isSubWindowON);
      }
#endif //ENABLE_FACE_DETECT
    }
  }
#endif


  if (alarmTimerCallbacked  && !mp3->isRunning()) {
    alarmTimerCallbacked = false;
#if defined(ENABLE_FACE_DETECT)
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
  }

  web_server_handle_client();
  ftpSrv.handleFTP();
  
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
      SST_ChatGPT();
    }
  }
#endif  //ENABLE_WAKEWORD

  if(xAlarmTimer != NULL){
    TickType_t xRemainingTime;

    /* Query the period of the timer that expires. */
    xRemainingTime = xTimerGetExpiryTime( xAlarmTimer ) - xTaskGetTickCount();
    avatarText = "残り" + String(xRemainingTime / 1000) + "秒";
    avatar.setSpeechText(avatarText.c_str());
  }
  //else{
  //  avatar.setSpeechText("");
  //}

#if defined( ENABLE_HEX_LED )
  if(recvMessages.size() > 0){
    hex_led_ptn_notification();
  }
#endif

  run_schedule();
  //reset_watchdog();
  
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
