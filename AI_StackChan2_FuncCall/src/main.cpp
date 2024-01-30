#include <Arduino.h>
//#include <FS.h>
#include <SD.h>
#include <SPIFFS.h>
#include <M5Unified.h>
#include <nvs.h>
#include <Avatar.h>

#include <AudioOutput.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include "AudioFileSourceHTTPSStream.h"
//#include "AudioFileSourceSD.h"
#include "AudioFileSourceSPIFFS.h"
#include "AudioOutputM5Speaker.h"
#include <ServoEasing.hpp> // https://github.com/ArminJo/ServoEasing       
#include "WebVoiceVoxTTS.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "rootCACertificate.h"
#include "rootCAgoogle.h"
#include <ArduinoJson.h>
#include "SpiRamJsonDocument.h"
#include <ESP32WebServer.h>
//#include <ESPmDNS.h>
#include "MailClient.h"
#include <ESP8266FtpServer.h>

//#include <deque>
#include "AudioWhisper.h"
#include "Whisper.h"
#include "Audio.h"
#include "CloudSpeechClient.h"
#if defined(ENABLE_WAKEWORD)
#include "WakeWord.h"
#endif

#include "FunctionCall.h"
#include "ChatHistory.h"

#if defined( ARDUINO_M5STACK_CORES3 )
#include "HexLED.h"


#if defined( ENABLE_FACE_DETECT )
#include <esp_camera.h>
#include <fb_gfx.h>
#include <vector>
#include "human_face_detect_msr01.hpp"
#include "human_face_detect_mnp01.hpp"
#define TWO_STAGE 1 /*<! 1: detect by two-stage which is more accurate but slower(with keypoints). */
                    /*<! 0: detect by one-stage which is less accurate but faster(without keypoints). */

#define FACE_COLOR_WHITE  0x00FFFFFF
#define FACE_COLOR_BLACK  0x00000000
#define FACE_COLOR_RED    0x000000FF
#define FACE_COLOR_GREEN  0x0000FF00
#define FACE_COLOR_BLUE   0x00FF0000
#define FACE_COLOR_YELLOW (FACE_COLOR_RED | FACE_COLOR_GREEN)
#define FACE_COLOR_CYAN   (FACE_COLOR_BLUE | FACE_COLOR_GREEN)
#define FACE_COLOR_PURPLE (FACE_COLOR_BLUE | FACE_COLOR_RED)

static camera_config_t camera_config = {
    .pin_pwdn     = -1,
    .pin_reset    = -1,
    .pin_xclk     = 2,
    .pin_sscb_sda = 12,
    .pin_sscb_scl = 11,

    .pin_d7 = 47,
    .pin_d6 = 48,
    .pin_d5 = 16,
    .pin_d4 = 15,
    .pin_d3 = 42,
    .pin_d2 = 41,
    .pin_d1 = 40,
    .pin_d0 = 39,

    .pin_vsync = 46,
    .pin_href  = 38,
    .pin_pclk  = 45,

    .xclk_freq_hz = 20000000,
    .ledc_timer   = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_RGB565,
    //.pixel_format = PIXFORMAT_JPEG,
    .frame_size   = FRAMESIZE_QVGA,   // QVGA(320x240)
    .jpeg_quality = 0,
    //.fb_count     = 2,
    .fb_count     = 1,
    .fb_location  = CAMERA_FB_IN_PSRAM,
    .grab_mode    = CAMERA_GRAB_WHEN_EMPTY,
};

bool isSubWindowON = true;
bool isSilentMode = false;

#endif    //ENABLE_FACE_DETECT


#endif

// 保存する質問と回答の最大数
//const int MAX_HISTORY = 5;
const int MAX_HISTORY = 20;

// 過去の質問と回答を保存するデータ構造
ChatHistory chatHistory(MAX_HISTORY);

#define USE_SDCARD
#define WIFI_SSID "SET YOUR WIFI SSID"
#define WIFI_PASS "SET YOUR WIFI PASS"
#define OPENAI_APIKEY "SET YOUR OPENAI APIKEY"
#define VOICEVOX_APIKEY "SET YOUR VOICEVOX APIKEY"
#define STT_APIKEY "SET YOUR STT APIKEY"

#define USE_SERVO
#ifdef USE_SERVO
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
#endif

// NTP接続情報　NTP connection information.
const char* NTPSRV      = "ntp.jst.mfeed.ad.jp";    // NTPサーバーアドレス NTP server address.
const long  GMT_OFFSET  = 9 * 3600;                 // GMT-TOKYO(時差９時間）9 hours time difference.
const int   DAYLIGHT_OFFSET = 0;                    // サマータイム設定なし No daylight saving time setting

/// 関数プロトタイプ宣言 /// 
void exec_chatGPT(String text);
void check_heap_free_size(void);
void check_heap_largest_free_block(void);

/// set M5Speaker virtual channel (0-7)
static constexpr uint8_t m5spk_virtual_channel = 0;

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

ESP32WebServer server(80);
FtpServer ftpSrv;   //set #define FTP_DEBUG in ESP8266FtpServer.h to see ftp verbose on serial

//---------------------------------------------
String OPENAI_API_KEY = "";
String VOICEVOX_API_KEY = "";
String STT_API_KEY = "";
String TTS_SPEAKER_NO = "3";
String TTS_SPEAKER = "&speaker=";
String TTS_PARMS = TTS_SPEAKER + TTS_SPEAKER_NO;

//---------------------------------------------
#if defined(ENABLE_WAKEWORD)
bool wakeword_is_enable = false;
#endif
//---------------------------------------------
// C++11 multiline string constants are neato...
static const char HEAD[] PROGMEM = R"KEWL(
<!DOCTYPE html>
<html lang="ja">
<head>
  <meta charset="UTF-8">
  <title>AIｽﾀｯｸﾁｬﾝ</title>
</head>)KEWL";

static const char APIKEY_HTML[] PROGMEM = R"KEWL(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <title>APIキー設定</title>
  </head>
  <body>
    <h1>APIキー設定</h1>
    <form>
      <label for="role1">OpenAI API Key</label>
      <input type="text" id="openai" name="openai" oninput="adjustSize(this)"><br>
      <label for="role2">VoiceVox API Key</label>
      <input type="text" id="voicevox" name="voicevox" oninput="adjustSize(this)"><br>
      <label for="role3">Speech to Text API Key</label>
      <input type="text" id="sttapikey" name="sttapikey" oninput="adjustSize(this)"><br>
      <button type="button" onclick="sendData()">送信する</button>
    </form>
    <script>
      function adjustSize(input) {
        input.style.width = ((input.value.length + 1) * 8) + 'px';
      }
      function sendData() {
        // FormDataオブジェクトを作成
        const formData = new FormData();

        // 各ロールの値をFormDataオブジェクトに追加
        const openaiValue = document.getElementById("openai").value;
        if (openaiValue !== "") formData.append("openai", openaiValue);

        const voicevoxValue = document.getElementById("voicevox").value;
        if (voicevoxValue !== "") formData.append("voicevox", voicevoxValue);

        const sttapikeyValue = document.getElementById("sttapikey").value;
        if (sttapikeyValue !== "") formData.append("sttapikey", sttapikeyValue);

	    // POSTリクエストを送信
	    const xhr = new XMLHttpRequest();
	    xhr.open("POST", "/apikey_set");
	    xhr.onload = function() {
	      if (xhr.status === 200) {
	        alert("データを送信しました！");
	      } else {
	        alert("送信に失敗しました。");
	      }
	    };
	    xhr.send(formData);
	  }
	</script>
  </body>
</html>)KEWL";

static const char ROLE_HTML[] PROGMEM = R"KEWL(
<!DOCTYPE html>
<html>
<head>
	<title>ロール設定</title>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<style>
		textarea {
			width: 80%;
			height: 200px;
			resize: both;
		}
	</style>
</head>
<body>
	<h1>ロール設定</h1>
	<form onsubmit="postData(event)">
		<label for="textarea">ここにロールを記述してください。:</label><br>
		<textarea id="textarea" name="textarea"></textarea><br><br>
		<input type="submit" value="Submit">
	</form>
	<script>
		function postData(event) {
			event.preventDefault();
			const textAreaContent = document.getElementById("textarea").value.trim();
//			if (textAreaContent.length > 0) {
				const xhr = new XMLHttpRequest();
				xhr.open("POST", "/role_set", true);
				xhr.setRequestHeader("Content-Type", "text/plain;charset=UTF-8");
			// xhr.onload = () => {
			// 	location.reload(); // 送信後にページをリロード
			// };
			xhr.onload = () => {
				document.open();
				document.write(xhr.responseText);
				document.close();
			};
				xhr.send(textAreaContent);
//        document.getElementById("textarea").value = "";
				alert("Data sent successfully!");
//			} else {
//				alert("Please enter some text before submitting.");
//			}
		}
	</script>
</body>
</html>)KEWL";



String speech_text = "";
String speech_text_buffer = "";
//DynamicJsonDocument chat_doc(1024*10);
//DynamicJsonDocument chat_doc(1024*30);
SpiRamJsonDocument chat_doc(0);     // PSRAMから確保するように変更。サイズの確保はsetup()で実施（初期化後でないとPSRAMが使えないため）。

// ChatGPTのJSONのテンプレートはFunctionCall.cppで定義
//String json_ChatString = "{\"model\": \"gpt-3.5-turbo-0613\",\"messages\": [{\"role\": \"user\", \"content\": \"""\"}]}";
//String Role_JSON = "";

bool init_chat_doc(const char *data)
{
  DeserializationError error = deserializeJson(chat_doc, data);
  if (error) {
    Serial.println("DeserializationError");

    String json_str; //= JSON.stringify(chat_doc);
    serializeJsonPretty(chat_doc, json_str);  // 文字列をシリアルポートに出力する
    Serial.println(json_str);

    return false;
  }
  String json_str; //= JSON.stringify(chat_doc);
  serializeJsonPretty(chat_doc, json_str);  // 文字列をシリアルポートに出力する
//  Serial.println(json_str);
    return true;
}

void handleRoot() {
  server.send(200, "text/plain", "hello from m5stack!");
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
//  server.send(404, "text/plain", message);
  server.send(404, "text/html", String(HEAD) + String("<body>") + message + String("</body>"));
}

void handle_speech() {
  String message = server.arg("say");
  String speaker = server.arg("voice");
  if(speaker != "") {
    TTS_PARMS = TTS_SPEAKER + speaker;
  }
  Serial.println(message);
  ////////////////////////////////////////
  // 音声の発声
  ////////////////////////////////////////
  avatar.setExpression(Expression::Happy);
  Voicevox_tts((char*)message.c_str(), (char*)TTS_PARMS.c_str());
  server.send(200, "text/plain", String("OK"));
}

String https_post_json(const char* url, const char* json_string, const char* root_ca) {
  String payload = "";
  WiFiClientSecure *client = new WiFiClientSecure;
  if(client) {
    client -> setCACert(root_ca);
    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
      HTTPClient https;
      https.setTimeout( 65000 ); 
  
      Serial.print("[HTTPS] begin...\n");
      if (https.begin(*client, url)) {  // HTTPS
        Serial.print("[HTTPS] POST...\n");
        // start connection and send HTTP header
        https.addHeader("Content-Type", "application/json");
        https.addHeader("Authorization", String("Bearer ") + OPENAI_API_KEY);
        int httpCode = https.POST((uint8_t *)json_string, strlen(json_string));
  
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
  
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            payload = https.getString();
            Serial.println("//////////////");
            Serial.println(payload);
            Serial.println("//////////////");
          }
        } else {
          Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }  
        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }
      // End extra scoping block
    }  
    delete client;
  } else {
    Serial.println("Unable to create client");
  }
  return payload;
}


String chatGpt(String json_string, String* calledFunc) {
  String response = "";
  avatar.setExpression(Expression::Doubt);
  avatar.setSpeechText("考え中…");
#if defined( ARDUINO_M5STACK_CORES3 )  
  hex_led_ptn_thinking_start();
#endif
  String ret = https_post_json("https://api.openai.com/v1/chat/completions", json_string.c_str(), root_ca_openai);
  avatar.setExpression(Expression::Neutral);
  avatar.setSpeechText("");
#if defined( ARDUINO_M5STACK_CORES3 )
  hex_led_ptn_thinking_end();
#endif
  Serial.println(ret);
  if(ret != ""){
    DynamicJsonDocument doc(2000);
    DeserializationError error = deserializeJson(doc, ret.c_str());
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      avatar.setExpression(Expression::Sad);
      avatar.setSpeechText("エラーです");
      response = "エラーです";
      delay(1000);
      avatar.setSpeechText("");
      avatar.setExpression(Expression::Neutral);
    }else{
      const char* data = doc["choices"][0]["message"]["content"];
      
      // content = nullならfunction call
      if(data == 0){
        response = exec_calledFunc(doc, calledFunc);
      }
      else{
        Serial.println(data);
        response = String(data);
        std::replace(response.begin(),response.end(),'\n',' ');
        *calledFunc = String("");
      }
    }
  } else {
    avatar.setExpression(Expression::Sad);
    avatar.setSpeechText("わかりません");
    response = "わかりません";
    delay(1000);
    avatar.setSpeechText("");
    avatar.setExpression(Expression::Neutral);
  }
  return response;
}

String InitBuffer = "";

void handle_chat() {
  static String response = "";
  // tts_parms_no = 1;
  String text = server.arg("text");
  String speaker = server.arg("voice");
  if(speaker != "") {
    TTS_PARMS = TTS_SPEAKER + speaker;
  }

  exec_chatGPT(text);

  server.send(200, "text/html", String(HEAD)+String("<body>")+response+String("</body>"));
}


#define MAX_REQUEST_COUNT  (10)
void exec_chatGPT(String text) {
  static String response = "";
  String calledFunc = "";
  String funcCallMode = "auto";

  //Serial.println(InitBuffer);
  //init_chat_doc(InitBuffer.c_str());

  // 質問をチャット履歴に追加
  chatHistory.push_back(String("user"), String(""), text);

  // functionの実行が要求されなくなるまで繰り返す
  for (int reqCount = 0; reqCount < MAX_REQUEST_COUNT; reqCount++)
  {
    init_chat_doc(InitBuffer.c_str());

    if(reqCount == (MAX_REQUEST_COUNT - 1)){
      funcCallMode = String("none");
    }

    for (int i = 0; i < chatHistory.get_size(); i++)
    {
      JsonArray messages = chat_doc["messages"];
      JsonObject systemMessage1 = messages.createNestedObject();

      if(chatHistory.get_role(i).equals(String("function"))){
        systemMessage1["role"] = chatHistory.get_role(i);
        systemMessage1["name"] = chatHistory.get_funcName(i);
        systemMessage1["content"] = chatHistory.get_content(i);
      }
      else{
        systemMessage1["role"] = chatHistory.get_role(i);
        systemMessage1["content"] = chatHistory.get_content(i);
      }

    }

    String json_string;
    serializeJson(chat_doc, json_string);

    //serializeJsonPretty(chat_doc, json_string);
    Serial.println("====================");
    Serial.println(json_string);
    Serial.println("====================");


    response = chatGpt(json_string, &calledFunc);

    // 返答をチャット履歴に追加
    if(calledFunc == ""){
      chatHistory.push_back(String("assistant"), String(""), response);
    }
    else{
      chatHistory.push_back(String("function"), calledFunc, response);      
    }

    if(calledFunc == ""){
      if(speech_text=="" && speech_text_buffer == "") {
        speech_text = response;
        Serial.println("Chat End");
      } else {
        //response = "busy";
        Serial.println("Chat End (TTS busy)");
      }

      //チャット履歴の容量を圧迫しないように、functionロールを削除する
      chatHistory.clean_function_role();

      break;
    }

  }
}

/*
String Role_JSON = "";
void exec_chatGPT1(String text) {
  static String response = "";
  init_chat_doc(Role_JSON.c_str());

  String role = chat_doc["messages"][0]["role"];
  if(role == "user") {chat_doc["messages"][0]["content"] = text;}
  String json_string;
  serializeJson(chat_doc, json_string);

  response = chatGpt(json_string);
  speech_text = response;
//  server.send(200, "text/html", String(HEAD)+String("<body>")+response+String("</body>"));
}
*/
void handle_apikey() {
  // ファイルを読み込み、クライアントに送信する
  server.send(200, "text/html", APIKEY_HTML);
}

void handle_apikey_set() {
  // POST以外は拒否
  if (server.method() != HTTP_POST) {
    return;
  }
  // openai
  String openai = server.arg("openai");
  // voicetxt
  String voicevox = server.arg("voicevox");
  // voicetxt
  String sttapikey = server.arg("sttapikey");
 
  OPENAI_API_KEY = openai;
  VOICEVOX_API_KEY = voicevox;
  STT_API_KEY = sttapikey;
  Serial.println(openai);
  Serial.println(voicevox);
  Serial.println(sttapikey);

  uint32_t nvs_handle;
  if (ESP_OK == nvs_open("apikey", NVS_READWRITE, &nvs_handle)) {
    nvs_set_str(nvs_handle, "openai", openai.c_str());
    nvs_set_str(nvs_handle, "voicevox", voicevox.c_str());
    nvs_set_str(nvs_handle, "sttapikey", sttapikey.c_str());
    nvs_close(nvs_handle);
  }
  server.send(200, "text/plain", String("OK"));
}

void handle_role() {
  // ファイルを読み込み、クライアントに送信する
  server.send(200, "text/html", ROLE_HTML);
}

bool save_json(){
  // SPIFFSをマウントする
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return false;
  }

  // JSONファイルを作成または開く
  File file = SPIFFS.open("/data.json", "w");
  if(!file){
    Serial.println("Failed to open file for writing");
    return false;
  }

  // JSONデータをシリアル化して書き込む
  serializeJson(chat_doc, file);
  file.close();
  return true;
}

/**
 * アプリからテキスト(文字列)と共にRoll情報が配列でPOSTされてくることを想定してJSONを扱いやすい形に変更
 * 出力形式をJSONに変更
*/
void handle_role_set() {
  // POST以外は拒否
  if (server.method() != HTTP_POST) {
    return;
  }
  String role = server.arg("plain");
  if (role != "") {
//    init_chat_doc(InitBuffer.c_str());
    init_chat_doc(json_ChatString.c_str());
    JsonArray messages = chat_doc["messages"];
    JsonObject systemMessage1 = messages.createNestedObject();
    systemMessage1["role"] = "system";
    systemMessage1["content"] = role;
//    serializeJson(chat_doc, InitBuffer);
  } else {
    init_chat_doc(json_ChatString.c_str());
  }
  //会話履歴をクリア
  chatHistory.clear();

  InitBuffer="";
  serializeJson(chat_doc, InitBuffer);
  Serial.println("InitBuffer = " + InitBuffer);
  //Role_JSON = InitBuffer;

  // JSONデータをspiffsへ出力する
  save_json();

  // 整形したJSONデータを出力するHTMLデータを作成する
  String html = "<html><body><pre>";
  serializeJsonPretty(chat_doc, html);
  html += "</pre></body></html>";

  // HTMLデータをシリアルに出力する
  Serial.println(html);
  server.send(200, "text/html", html);
//  server.send(200, "text/plain", String("OK"));
};

// 整形したJSONデータを出力するHTMLデータを作成する
void handle_role_get() {

  String html = "<html><body><pre>";
  serializeJsonPretty(chat_doc, html);
  html += "</pre></body></html>";

  // HTMLデータをシリアルに出力する
  Serial.println(html);
  server.send(200, "text/html", String(HEAD) + html);
};

void handle_face() {
  String expression = server.arg("expression");
  expression = expression + "\n";
  Serial.println(expression);
  switch (expression.toInt())
  {
    case 0: avatar.setExpression(Expression::Neutral); break;
    case 1: avatar.setExpression(Expression::Happy); break;
    case 2: avatar.setExpression(Expression::Sleepy); break;
    case 3: avatar.setExpression(Expression::Doubt); break;
    case 4: avatar.setExpression(Expression::Sad); break;
    case 5: avatar.setExpression(Expression::Angry); break;  
  } 
  server.send(200, "text/plain", String("OK"));
}

void handle_setting() {
  String value = server.arg("volume");
  String led = server.arg("led");
  String speaker = server.arg("speaker");
//  volume = volume + "\n";
  Serial.println(speaker);
  Serial.println(value);
  size_t speaker_no;
  if(speaker != ""){
    speaker_no = speaker.toInt();
    if(speaker_no > 60) {
      speaker_no = 60;
    }
    TTS_SPEAKER_NO = String(speaker_no);
    TTS_PARMS = TTS_SPEAKER + TTS_SPEAKER_NO;
  }

  if(value == "") value = "180";
  size_t volume = value.toInt();
  uint8_t led_onoff = 0;
  uint32_t nvs_handle;
  if (ESP_OK == nvs_open("setting", NVS_READWRITE, &nvs_handle)) {
    if(volume > 255) volume = 255;
    nvs_set_u32(nvs_handle, "volume", volume);
    if(led != "") {
      if(led == "on") led_onoff = 1;
      else  led_onoff = 0;
      nvs_set_u8(nvs_handle, "led", led_onoff);
    }
    nvs_set_u8(nvs_handle, "speaker", speaker_no);

    nvs_close(nvs_handle);
  }
  M5.Speaker.setVolume(volume);
  M5.Speaker.setChannelVolume(m5spk_virtual_channel, volume);
  server.send(200, "text/plain", String("OK"));
}

AudioOutputM5Speaker out(&M5.Speaker, m5spk_virtual_channel);
AudioGeneratorMP3 *mp3;
AudioFileSourceBuffer *buff = nullptr;
int preallocateBufferSize = 30*1024;
uint8_t *preallocateBuffer;
AudioFileSourceHTTPSStream *file = nullptr;
//AudioFileSourceSD *file_sd = nullptr;
AudioFileSourceSPIFFS *file_sd = nullptr;

void playMP3(AudioFileSourceBuffer *buff){
  mp3->begin(buff, &out);
}

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

bool servo_home = false;

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
  if (servo_x.attach(SERVO_PIN_X, START_DEGREE_VALUE_X, DEFAULT_MICROSECONDS_FOR_0_DEGREE, DEFAULT_MICROSECONDS_FOR_180_DEGREE)) {
    Serial.print("Error attaching servo x");
  }
  if (servo_y.attach(SERVO_PIN_Y, START_DEGREE_VALUE_Y, DEFAULT_MICROSECONDS_FOR_0_DEGREE, DEFAULT_MICROSECONDS_FOR_180_DEGREE)) {
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



#if defined(ENABLE_FACE_DETECT)
esp_err_t camera_init(){

    //initialize the camera
    M5.In_I2C.release();
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        //Serial.println("Camera Init Failed");
        M5.Display.println("Camera Init Failed");
        return err;
    }

    return ESP_OK;
}

static void draw_face_boxes(fb_data_t *fb, std::list<dl::detect::result_t> *results, int face_id)
{
    int x, y, w, h;
    uint32_t color = FACE_COLOR_YELLOW;
    if (face_id < 0)
    {
        color = FACE_COLOR_RED;
    }
    else if (face_id > 0)
    {
        color = FACE_COLOR_GREEN;
    }
    if(fb->bytes_per_pixel == 2){
        //color = ((color >> 8) & 0xF800) | ((color >> 3) & 0x07E0) | (color & 0x001F);
        color = ((color >> 16) & 0x001F) | ((color >> 3) & 0x07E0) | ((color << 8) & 0xF800);
    }
    int i = 0;
    for (std::list<dl::detect::result_t>::iterator prediction = results->begin(); prediction != results->end(); prediction++, i++)
    {
        // rectangle box
        x = (int)prediction->box[0];
        y = (int)prediction->box[1];

        // yが負の数のときにfb_gfx_drawFastHLine()でメモリ破壊してリセットする不具合の対策
        if(y < 0){
           y = 0;
        }

        w = (int)prediction->box[2] - x + 1;
        h = (int)prediction->box[3] - y + 1;

        //Serial.printf("x:%d y:%d w:%d h:%d\n", x, y, w, h);

        if((x + w) > fb->width){
            w = fb->width - x;
        }
        if((y + h) > fb->height){
            h = fb->height - y;
        }

        //Serial.printf("x:%d y:%d w:%d h:%d\n", x, y, w, h);

        //fb_gfx_fillRect(fb, x+10, y+10, w-20, h-20, FACE_COLOR_RED);  //モザイク
        fb_gfx_drawFastHLine(fb, x, y, w, color);
        fb_gfx_drawFastHLine(fb, x, y + h - 1, w, color);
        fb_gfx_drawFastVLine(fb, x, y, h, color);
        fb_gfx_drawFastVLine(fb, x + w - 1, y, h, color);

#if TWO_STAGE
        // landmarks (left eye, mouth left, nose, right eye, mouth right)
        int x0, y0, j;
        for (j = 0; j < 10; j+=2) {
            x0 = (int)prediction->keypoint[j];
            y0 = (int)prediction->keypoint[j+1];
            fb_gfx_fillRect(fb, x0, y0, 3, 3, color);
        }
#endif
    }
}


void debug_check_heap_free_size(void){
  Serial.printf("===============================================================\n");
  Serial.printf("Mem Test\n");
  Serial.printf("===============================================================\n");
  Serial.printf("esp_get_free_heap_size()                              : %6d\n", esp_get_free_heap_size() );
  Serial.printf("heap_caps_get_free_size(MALLOC_CAP_DMA)               : %6d\n", heap_caps_get_free_size(MALLOC_CAP_DMA) );
  Serial.printf("heap_caps_get_free_size(MALLOC_CAP_SPIRAM)            : %6d\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM) );
  Serial.printf("heap_caps_get_free_size(MALLOC_CAP_INTERNAL)          : %6d\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL) );
  Serial.printf("heap_caps_get_free_size(MALLOC_CAP_DEFAULT)           : %6d\n", heap_caps_get_free_size(MALLOC_CAP_DEFAULT) );

}


bool camera_capture_and_face_detect(){
  bool isDetected = false;

  //acquire a frame
  M5.In_I2C.release();
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    //Serial.println("Camera Capture Failed");
    M5.Display.println("Camera Capture Failed");
    return ESP_FAIL;
  }


  int face_id = 0;

#if TWO_STAGE
  HumanFaceDetectMSR01 s1(0.1F, 0.5F, 10, 0.2F);
  HumanFaceDetectMNP01 s2(0.5F, 0.3F, 5);
  std::list<dl::detect::result_t> &candidates = s1.infer((uint16_t *)fb->buf, {(int)fb->height, (int)fb->width, 3});
  std::list<dl::detect::result_t> &results = s2.infer((uint16_t *)fb->buf, {(int)fb->height, (int)fb->width, 3}, candidates);
#else
  HumanFaceDetectMSR01 s1(0.3F, 0.5F, 10, 0.2F);
  std::list<dl::detect::result_t> &results = s1.infer((uint16_t *)fb->buf, {(int)fb->height, (int)fb->width, 3});
#endif


  
  if (results.size() > 0) {
    //Serial.printf("Face detected : %d\n", results.size());

    isDetected = true;

    fb_data_t rfb;
    rfb.width = fb->width;
    rfb.height = fb->height;
    rfb.data = fb->buf;
    rfb.bytes_per_pixel = 2;
    rfb.format = FB_RGB565;

    draw_face_boxes(&rfb, &results, face_id);

  }

  if(isSubWindowON){
    avatar.updateSubWindow(fb->buf);
  }

  //Serial.println("\n<<< heap size before fb return >>>");  
  //debug_check_heap_free_size();

  //return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);

  //Serial.println("<<< heap size after fb return >>>");  
  //debug_check_heap_free_size();

  return isDetected;
}
#endif  //ENABLE_FACE_DETECT


// SDカードのファイルをSPIFFSにコピー
bool copySDFileToSPIFFS(const char *path, bool forced) {
  
  Serial.println("Copy SD File to SPIFFS.");

  if(!SPIFFS.begin(true)){
    Serial.println("Failed to mount SPIFFS.");
    return false;
  }

  if (SPIFFS.exists(path) && !forced) {
	  Serial.println("File already exists in SPIFFS.");
	  return true;
  }

  if (!SD.begin(GPIO_NUM_4, SPI, 25000000)) {
    Serial.println("Failed to mount SD.");
    return false;
  }

  File fsrc = SD.open("/alarm.mp3", FILE_READ);
  File fdst = SPIFFS.open("/alarm.mp3", FILE_WRITE);
  if(!fsrc || !fdst) {
    Serial.println("Failed to open file.");
    return false;
  }

  uint8_t *buf = new uint8_t[4096];
  if (!buf) {
	  Serial.println("Failed to allocate buffer.");
	  return false;
  }

  size_t len, size, ret;
  size = len = fsrc.size();
  while (len) {
	  size_t s = len;
	  if (s > 4096){
		  s = 4096;
    }  

	  fsrc.read(buf, s);
	  if ((ret = fdst.write(buf, s)) < s) {
		  Serial.printf("write failed: %d - %d\n", ret, s);
		  return false;
	  }
	  len -= s;
	  Serial.printf("%d / %d\n", size - len, size);
  }
 
  delete[] buf;
  fsrc.close();
  fdst.close();

  if (!SPIFFS.exists(path)) {
	  Serial.println("no file in SPIFFS.");
	  return false;
  }
  fdst = SPIFFS.open(path);
  len = fdst.size();
  fdst.close();
  if (len != size) {
	 Serial.println("size not match.");
	 return false;
  }
  Serial.println("*** Done. ***\r\n");
  return true;
}

void setup()
{
  auto cfg = M5.config();

  cfg.external_spk = true;    /// use external speaker (SPK HAT / ATOMIC SPK)
//cfg.external_spk_detail.omit_atomic_spk = true; // exclude ATOMIC SPK
//cfg.external_spk_detail.omit_spk_hat    = true; // exclude SPK HAT
//  cfg.output_power = true;
  M5.begin(cfg);

  chat_doc = SpiRamJsonDocument(1024*30);

  //TTS MP3用バッファ （PSRAMから確保される）
  preallocateBuffer = (uint8_t *)malloc(preallocateBufferSize);
  if (!preallocateBuffer) {
    M5.Display.printf("FATAL ERROR:  Unable to preallocate %d bytes for app\n", preallocateBufferSize);
    for (;;) { delay(1000); }
  }

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
    auto fs = SD.open("/wifi.txt", FILE_READ);
    if(fs) {
      size_t sz = fs.size();
      char buf[sz + 1];
      fs.read((uint8_t*)buf, sz);
      buf[sz] = 0;
      fs.close();

      int y = 0;
      for(int x = 0; x < sz; x++) {
        if(buf[x] == 0x0a || buf[x] == 0x0d)
          buf[x] = 0;
        else if (!y && x > 0 && !buf[x - 1] && buf[x])
          y = x;
      }
      WiFi.begin(buf, &buf[y]);
    } else {
       WiFi.begin();
    }

    uint32_t nvs_handle;
    if (ESP_OK == nvs_open("apikey", NVS_READWRITE, &nvs_handle)) {
      /// radiko-premium
      fs = SD.open("/apikey.txt", FILE_READ);
      if(fs) {
        size_t sz = fs.size();
        char buf[sz + 1];
        fs.read((uint8_t*)buf, sz);
        buf[sz] = 0;
        fs.close();
  
        int y = 0;
        int z = 0;
        for(int x = 0; x < sz; x++) {
          if(buf[x] == 0x0a || buf[x] == 0x0d)
            buf[x] = 0;
          else if (!y && x > 0 && !buf[x - 1] && buf[x])
            y = x;
          else if (!z && x > 0 && !buf[x - 1] && buf[x])
            z = x;
        }

        nvs_set_str(nvs_handle, "openai", buf);
        nvs_set_str(nvs_handle, "voicevox", &buf[y]);
        nvs_set_str(nvs_handle, "sttapikey", &buf[z]);
        Serial.println("------------------------");
        Serial.println(buf);
        Serial.println(&buf[y]);
        Serial.println(&buf[z]);
        Serial.println("------------------------");
      }
      
      nvs_close(nvs_handle);
    }

    /// Weather city ID
    fs = SD.open("/weather_city_id.txt", FILE_READ);
    if(fs) {
      size_t sz = fs.size();
      char buf[sz + 1];
      fs.read((uint8_t*)buf, sz);
      buf[sz] = 0;
      fs.close();

      for(int x = 0; x < sz; x++) {
        if(buf[x] == 0x0a || buf[x] == 0x0d){
          buf[x] = 0;
        }
      }
      Serial.println(buf);
      weatherCityID = String(buf);
    } else {
      weatherCityID = "130010";
    }
    Serial.println(weatherCityID);


    // Gmailアカウント、アプリパスワード
    fs = SD.open("/gmail.txt", FILE_READ);
    if(fs) {
      size_t sz = fs.size();
      char buf[sz + 1];
      fs.read((uint8_t*)buf, sz);
      buf[sz] = 0;
      fs.close();

      int y = 0;
      int z = 0;
      for(int x = 0; x < sz; x++) {
        if(buf[x] == 0x0a || buf[x] == 0x0d)
          buf[x] = 0;
        else if (!y && x > 0 && !buf[x - 1] && buf[x])
          y = x;
        else if (!z && x > 0 && !buf[x - 1] && buf[x])
          z = x;
      }

      authMailAdr = String(buf);
      authAppPass = String(&buf[y]);
      toMailAdr = String(&buf[z]);

      Serial.println("My mail addr: " + authMailAdr);
      Serial.println("My mail pass: " + authAppPass);
      Serial.println("To mail addr: " + toMailAdr);

    }
    else {
      Serial.println("Failed to SD.open().");    
    }


    /// メモがあるか確認
    fs = SD.open("/notepad.txt", FILE_READ);
    if(fs) {
      size_t sz = fs.size();
      char buf[sz + 1];
      fs.read((uint8_t*)buf, sz);
      buf[sz] = 0;
      fs.close();

      note = String(buf);
      Serial.printf("Notepad size: %d\n", sz);

      if(sz >= 1){
        speech_text = "メモが保存されています。";
        //chatHistory.push_back(String("assistant"), String(""), speech_text);
      }
    }
    else {
      Serial.println("Failed to SD.open().");    
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

  server.on("/", handleRoot);

  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  // And as regular external functions:
  server.on("/speech", handle_speech);
  server.on("/face", handle_face);
  server.on("/chat", handle_chat);
  server.on("/apikey", handle_apikey);
  server.on("/setting", handle_setting);
  server.on("/apikey_set", HTTP_POST, handle_apikey_set);
  server.on("/role", handle_role);
  server.on("/role_set", HTTP_POST, handle_role_set);
  server.on("/role_get", handle_role_get);
  server.onNotFound(handleNotFound);

  init_chat_doc(json_ChatString.c_str());
  // SPIFFSをマウントする
  if(SPIFFS.begin(true)){
    // JSONファイルを開く
    File file = SPIFFS.open("/data.json", "r");
    if(file){
      DeserializationError error = deserializeJson(chat_doc, file);
      if(error){
        Serial.println("Failed to deserialize JSON");
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

  server.begin();
  Serial.println("HTTP server started");
  M5.Lcd.println("HTTP server started");  

  ftpSrv.begin("stackchan","stackchan");    //username, password for ftp.  set ports in ESP8266FtpServer.h  (default 21, 50009 for PASV)
  Serial.println("FTP server started");
  M5.Lcd.println("FTP server started");  


  Serial.printf_P(PSTR("/ to control the chatGpt Server.\n"));
  M5.Lcd.print("/ to control the chatGpt Server.\n");
  delay(3000);

  audioLogger = &Serial;
  mp3 = new AudioGeneratorMP3();
//  mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");

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
  startReadMailTimer();

#if defined( ARDUINO_M5STACK_CORES3 )
  hex_led_init();
  //hex_led_ptn_off();
  hex_led_ptn_boot();
#if defined(ENABLE_FACE_DETECT)
  camera_init();
  avatar.set_isSubWindowEnable(true);
#endif
#endif


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
  speech_text = String(buff);
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
    speech_text = tmp;
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

#if defined( ARDUINO_M5STACK_CORES3 )
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
  servo_home = prev_servo_home;
#endif
  Serial.println("音声認識終了");
  Serial.println("音声認識結果");
  if(ret != "") {
    Serial.println(ret);
    if (!mp3->isRunning() && speech_text=="" && speech_text_buffer == "") {
#if defined( ARDUINO_M5STACK_CORES3 )
      hex_led_ptn_accept();
#endif
      exec_chatGPT(ret);
#if defined(ENABLE_WAKEWORD)
      mode = 0;
#endif
    }
  } else {
#if defined( ARDUINO_M5STACK_CORES3 )
    hex_led_ptn_off();
#endif
    Serial.println("音声認識失敗");
    avatar.setExpression(Expression::Sad);
    avatar.setSpeechText("聞き取れませんでした");
    delay(2000);
    avatar.setSpeechText("");
    avatar.setExpression(Expression::Neutral);
  } 
}

void loop()
{
  static int lastms = 0;

  if (random_time >= 0 && millis() - lastms1 > random_time)
  {
    lastms1 = millis();
    random_time = 40000 + 1000 * random(30);
    if (!mp3->isRunning() && speech_text=="" && speech_text_buffer == "") {
      exec_chatGPT(random_words[random(18)]);
#if defined(ENABLE_WAKEWORD)
      mode = 0;
#endif
    }
  }

  
#if defined(ENABLE_FACE_DETECT)
  //しゃべっていないときに顔検出を実行し、顔が検出されれば音声認識を開始。
  if (!mp3->isRunning() && speech_text=="" && speech_text_buffer == "") {
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

  }
#endif

  if(speech_text=="" && speech_text_buffer == ""){
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
          servo_home = !servo_home;
          //M5.Speaker.tone(1000, 100);
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
  }

  if(speech_text != ""){
    avatar.setExpression(Expression::Happy);
    speech_text_buffer = speech_text;
    speech_text = "";
    M5.Mic.end();
    M5.Speaker.begin();
#if defined(ENABLE_WAKEWORD)
    mode = 0;
#endif
    Voicevox_tts((char*)speech_text_buffer.c_str(), (char*)TTS_PARMS.c_str());
  }


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
      //file_sd = new AudioFileSourceSD("/alarm.mp3");
      file_sd = new AudioFileSourceSPIFFS("/alarm.mp3");
      Serial.println("Open mp3");
      
      if( !file_sd->isOpen() ){
        delete file_sd;
        file_sd = nullptr;
        //Serial.println("failed to open mp3 file");
        //mp3がない場合はｽﾀｯｸﾁｬﾝがしゃべる
        speech_text = "時間になりました。";
      }
      else{
        avatar.setExpression(Expression::Happy);
        M5.Mic.end();
        M5.Speaker.begin();
#if defined(ENABLE_WAKEWORD)
        mode = 0;
#endif
        mp3->begin(file_sd, &out);
      }
    }
  }

  //if (mp3->isRunning()) {
  while(mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
      if(file != nullptr){delete file; file = nullptr;}
      if(buff != nullptr){delete buff; buff = nullptr;}
      Serial.println("mp3 stop");
      avatar.setExpression(Expression::Neutral);
      speech_text_buffer = "";
      delay(200);
      M5.Speaker.end();
      M5.Mic.begin();

      //SDカードのmp3ファイルを再生している場合
      if(file_sd != nullptr){
        delete file_sd;
        file_sd = nullptr;
      }

#if defined(ENABLE_WAKEWORD)
      if(wakeword_is_enable) mode = 1;
      else  mode = 0;
#endif

#if defined(ENABLE_FACE_DETECT)
      if(isSubWindowON){
        avatar.set_isSubWindowEnable(true);
      }
#endif  //ENABLE_FACE_DETECT
    }
    delay(1);
//  } else {
//    server.handleClient();
  }
  server.handleClient();
  ftpSrv.handleFTP();
  
#if defined(ENABLE_WAKEWORD)
  if (mode == 0) { /* return; */ }
  else if (mode < 0) {
    if(wakeword_regist()){
      avatar.setSpeechText("ウェイクワード登録終了");
      delay(1000);
      avatar.setSpeechText("");
      mode = 0;
      wakeword_is_enable = false;
#ifdef USE_SERVO
      servo_home = false;
#endif
    }
  }
  else if (mode > 0 && wakeword_is_enable) {
    if( wakeword_compare()){
        Serial.println("wakeword_compare OK!");
        sw_tone();
        SST_ChatGPT();
    }
  }
#endif  //ENABLE_WAKEWORD

  if(readMailTimerCallbacked && !mp3->isRunning()){
    avatar.setSpeechText("受信メール確認中");
    Serial.println("loop task: imapReadMail()");
    imapReadMail();
    readMailTimerCallbacked = false;
    avatar.setSpeechText("");

    int nMail = recvMessages.size();
    if(nMail > prev_nMail){
      if(speech_text=="" && speech_text_buffer == "") {
        speech_text = String(nMail) + "件のメールを受信しています。";
        prev_nMail = nMail;
      }
    }

    //ヒープメモリ残量確認(デバッグ用)
    check_heap_free_size();
    check_heap_largest_free_block();
  }

#if defined( ARDUINO_M5STACK_CORES3 )
  if(recvMessages.size() > 0){
    hex_led_ptn_notification();
  }
#endif

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



 
