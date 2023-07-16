#include "FunctionCall.h"
#include <Avatar.h>
#include <AudioGeneratorMP3.h>
using namespace m5avatar;

extern Avatar avatar;
extern String speech_text;
extern String speech_text_buffer;
extern AudioGeneratorMP3 *mp3;

TimerHandle_t xTimer, xOffTimer;
void alarmTimerCallback(TimerHandle_t xTimer);
void powerOffTimerCallback(TimerHandle_t xTimer);

static String stackchan_timer(int32_t time, const char* action);
static String stackchan_timer_change_period(int32_t time);

String json_ChatString = 
"{\"model\": \"gpt-3.5-turbo-0613\","
"\"messages\": [{\"role\": \"user\", \"content\": \"\"}],"
"\"functions\": ["
  "{"
    "\"name\": \"stackchan_timer\","
    "\"description\": \"指定した時間が経過したら、指定した動作を実行する。指定できる動作はalarmとshutdown。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {"
        "\"time\":{"
          "\"type\": \"integer\","
          "\"description\": \"指定したい時間。単位は秒。\""
        "},"
        "\"action\":{"
          "\"type\": \"string\","
          "\"description\": \"指定したい動作。alarmは音で通知。shutdownは電源OFF。\","
          "\"enum\": [\"alarm\", \"shutdown\"]"
        "}"
      "},"
      "\"required\": [\"time\", \"action\"]"
    "}"
  "},"
  "{"
    "\"name\": \"stackchan_timer_change_period\","
    "\"description\": \"実行中のタイマーの設定時間を変更する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {"
        "\"time\":{"
          "\"type\": \"integer\","
          "\"description\": \"変更後の時間。単位は秒。0の場合はタイマーをキャンセルする。\""
        "}"
      "},"
      "\"required\": [\"time\"]"
    "}"
  "},"
  "{"
    "\"name\": \"get_time\","
    "\"description\": \"現在の時刻を取得する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {}"
    "}"
  "},"
  "{"
    "\"name\": \"listen_to_you\","
    "\"description\": \"相手がこれから話すことを聞き取る。例えば、「これから言うことをメモして」と言われた時や、相手の発言を促したいときにこの関数を使用する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {}"
    "}"
  "},"
  "{"
    "\"name\": \"save_note\","
    "\"description\": \"メモを保存する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {"
        "\"text\":{"
          "\"type\": \"string\","
          "\"description\": \"メモの内容。\""
        "}"
      "}"
    "}"
  "},"
  "{"
    "\"name\": \"read_note\","
    "\"description\": \"メモを読み上げる。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {}"
    "}"
  "}"
"],"
"\"function_call\":\"auto\""
"}";


void alarmTimerCallback(TimerHandle_t _xTimer){
  xTimer = NULL;
  Serial.println("時間になりました。");
  if (!mp3->isRunning() && speech_text=="" && speech_text_buffer == "") {
    speech_text = "時間になりました。";
  }
}

void powerOffTimerCallback(TimerHandle_t _xTimer){
  xTimer = NULL;
  Serial.println("おやすみなさい。");
  avatar.setSpeechText("おやすみなさい。");
  delay(2000);
  avatar.setSpeechText("");
  M5.Power.powerOff();
}

String stackchan_timer(int32_t time, const char* action){
  String response = "";

  if(xTimer != NULL){
    response = "別のタイマーを実行中です。";
  }
  else{
    if(strcmp(action, "alarm") == 0){
      xTimer = xTimerCreate("Timer", time * 1000, pdFALSE, 0, alarmTimerCallback);
      if(xTimer != NULL){
        xTimerStart(xTimer, 0);
        response = String(time) + "秒後にタイマーをセットしました。";
      }
      else{
        response = String(time) + "タイマー設定が失敗しました。";
      }
    }
    else if(strcmp(action, "shutdown") == 0){
      xTimer = xTimerCreate("Timer", time * 1000, pdFALSE, 0, powerOffTimerCallback);
      if(xTimer != NULL){
        xTimerStart(xTimer, 0);
        response = String(time) + "秒後にパワーオフします。";
      }
      else{
        response = String(time) + "タイマー設定が失敗しました。";
      }
    }
  }

  Serial.println(response);
  return response;
}

String stackchan_timer_change_period(int32_t time){
  String response = "";
  if(time == 0){
    xTimerDelete(xTimer, 0);
    xTimer = NULL;
    response = "タイマーをキャンセルしました。";
  }
  else{
    xTimerChangePeriod(xTimer, time * 1000, 0);
    response = "タイマーの設定時間を" + String(time) + "秒に変更しました。";
  }

  return response;
}

String get_time(){
  String response = "";
  struct tm timeInfo; 

  if (getLocalTime(&timeInfo)) {                            // timeinfoに現在時刻を格納
    response = String(timeInfo.tm_hour) + "時" + String(timeInfo.tm_min) + "分です。";
  }
  else{
    response = "時刻取得に失敗しました。";
  }
  return response;
}

extern bool servo_home;
extern String OPENAI_API_KEY;
extern String STT_API_KEY;
extern String SpeechToText(bool isGoogle);

String listen_to_you(){
  M5.Speaker.tone(1000, 100);
  delay(200);
  M5.Speaker.end();
  bool prev_servo_home = servo_home;
//#ifdef USE_SERVO
  servo_home = true;
//#endif
  avatar.setExpression(Expression::Happy);
  avatar.setSpeechText("どうぞ話してください");
  M5.Speaker.end();
  String ret;

  if(OPENAI_API_KEY != STT_API_KEY){
    Serial.println("Google STT");
    ret = SpeechToText(true);
  } else {
    Serial.println("Whisper STT");
    ret = SpeechToText(false);
  }

//#ifdef USE_SERVO
  servo_home = prev_servo_home;
//#endif
  Serial.println("音声認識終了");
  if(ret != "") {
    Serial.println(ret);

  } else {
    Serial.println("音声認識失敗");
    avatar.setExpression(Expression::Sad);
    avatar.setSpeechText("聞き取れませんでした");
    delay(2000);

  } 

  avatar.setSpeechText("");
  avatar.setExpression(Expression::Neutral);
  M5.Speaker.begin();

  return ret;
}

String note = "";
String save_note(const char* text){
  note = String(text);
  String response = "メモを保存しました。";
  return response;
}

String read_note(){
  return String("メモの内容は次の通り。") + note;
}

String exec_calledFunc(DynamicJsonDocument doc, String* calledFunc){
  String response = "";
  const char* name = doc["choices"][0]["message"]["function_call"]["name"];
  const char* args = doc["choices"][0]["message"]["function_call"]["arguments"];

  Serial.println(name);
  Serial.println(args);

  DynamicJsonDocument argsDoc(200);
  DeserializationError error = deserializeJson(argsDoc, args);
  if (error) {
    Serial.print(F("deserializeJson(arguments) failed: "));
    Serial.println(error.f_str());
    avatar.setExpression(Expression::Sad);
    avatar.setSpeechText("エラーです");
    response = "エラーです";
    delay(1000);
    avatar.setSpeechText("");
    avatar.setExpression(Expression::Neutral);
  }else{
    *calledFunc = String(name);

    if(strcmp(name, "stackchan_timer") == 0){
      const int time = argsDoc["time"];
      const char* action = argsDoc["action"];
      Serial.printf("time:%d\n",time);
      Serial.println(action);

      response = stackchan_timer(time, action);
    }
    else if(strcmp(name, "stackchan_timer_change_period") == 0){
      const int time = argsDoc["time"];
      response = stackchan_timer_change_period(time);    
    }
    else if(strcmp(name, "get_time") == 0){
      response = get_time();    
    }
    else if(strcmp(name, "listen_to_you") == 0){
      response = listen_to_you();    
    }
    else if(strcmp(name, "save_note") == 0){
      const char* text = argsDoc["text"];
      Serial.println(text);
      response = save_note(text);
    }
    else if(strcmp(name, "read_note") == 0){
      response = read_note();    
    }
  }

  return response;
}
