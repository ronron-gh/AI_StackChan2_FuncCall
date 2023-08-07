#include "FunctionCall.h"
#include <Avatar.h>
#include <AudioGeneratorMP3.h>
#include <HTTPClient.h>
#include <SD.h>
#include <EMailSender.h>
#include "MailClient.h"
using namespace m5avatar;

extern Avatar avatar;
extern String speech_text;
extern String speech_text_buffer;
extern AudioGeneratorMP3 *mp3;

TimerHandle_t xTimer, xOffTimer;
void alarmTimerCallback(TimerHandle_t xTimer);
void powerOffTimerCallback(TimerHandle_t xTimer);

static String timer(int32_t time, const char* action);
static String timer_change(int32_t time);

String json_ChatString = 
"{\"model\": \"gpt-3.5-turbo-16k-0613\","
//"{\"model\": \"gpt-4-0613\","
"\"messages\": [{\"role\": \"user\", \"content\": \"\"}],"
"\"functions\": ["
  "{"
    "\"name\": \"timer\","
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
    "\"name\": \"timer_change\","
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
    "\"name\": \"get_date\","
    "\"description\": \"今日の日付を取得する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {}"
    "}"
  "},"
  "{"
    "\"name\": \"get_time\","
    "\"description\": \"現在の時刻を取得する。タイマー設定したり時刻表を調べたりする前にこの関数で現在時刻を調べる。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {}"
    "}"
  "},"
  "{"
    "\"name\": \"get_week\","
    "\"description\": \"今日が何曜日かを取得する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {}"
    "}"
  "},"
  "{"
    "\"name\": \"listen\","
    "\"description\": \"相手がこれから話すことを聞き取る。例えば、「これから言うことをメモして」と言われたときにこの関数を使用する。\","
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
  "},"
  "{"
    "\"name\": \"delete_note\","
    "\"description\": \"メモを消去する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {}"
    "}"
  "},"
  "{"
    "\"name\": \"get_bus_time\","
    "\"description\": \"次のバス（または電車）の時刻を取得する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {"
        "\"nNext\":{"
          "\"type\": \"integer\","
          "\"description\": \"次の発車時刻を取得する場合は0にする。次の次の発車時刻を取得する場合は1にする。\""
        "}"
      "},"
      "\"required\": [\"nNext\"]"
    "}"
  "},"
  "{"
    "\"name\": \"send_mail\","
    "\"description\": \"メールを送信する。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {"
        "\"message\":{"
          "\"type\": \"string\","
          "\"description\": \"メールの内容。\""
        "}"
      "}"
    "}"
  "},"
  "{"
    "\"name\": \"read_mail\","
    "\"description\": \"受信メールを読み上げる。\","
    "\"parameters\": {"
      "\"type\":\"object\","
      "\"properties\": {}"
    "}"
  "},"
  "{"
    "\"name\": \"get_weathers\","
    "\"description\": \"天気予報を取得。\","
    "\"parameters\":  {"
      "\"type\":\"object\","
      "\"properties\": {},"
      "\"required\": []"
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

String timer(int32_t time, const char* action){
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

String timer_change(int32_t time){
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


String get_date(){
  String response = "";
  struct tm timeInfo; 

  if (getLocalTime(&timeInfo)) {                            // timeinfoに現在時刻を格納
    response = String(timeInfo.tm_year + 1900) + "年"
               + String(timeInfo.tm_mon + 1) + "月"
               + String(timeInfo.tm_mday) + "日";
  }
  else{
    response = "時刻取得に失敗しました。";
  }
  return response;
}

String get_time(){
  String response = "";
  struct tm timeInfo; 

  if (getLocalTime(&timeInfo)) {                            // timeinfoに現在時刻を格納
    response = String(timeInfo.tm_hour) + "時" + String(timeInfo.tm_min) + "分";
  }
  else{
    response = "時刻取得に失敗しました。";
  }
  return response;
}


String get_week(){
  String response = "";
  struct tm timeInfo; 
  const char week[][8] = {"日", "月", "火", "水", "木", "金", "土"};

  if (getLocalTime(&timeInfo)) {                            // timeinfoに現在時刻を格納
    response = String(week[timeInfo.tm_wday]) + "曜日";
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
extern void sw_tone();

String listen(){
  sw_tone();
  bool prev_servo_home = servo_home;
//#ifdef USE_SERVO
  servo_home = true;
//#endif
  avatar.setExpression(Expression::Happy);
  avatar.setSpeechText("どうぞ話してください");
  //M5.Speaker.end();
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
  //M5.Speaker.begin();

  return ret;
}

String note = "";
String save_note(const char* text){
  String response = "";
  
  if (SD.begin(GPIO_NUM_4, SPI, 25000000)) {
    auto fs = SD.open("/notepad.txt", FILE_WRITE, true);

    if(fs) {
      if(note == ""){
        note = String(text);
      }
      else{
        note = note + "\n" + String(text);
      }
      Serial.println(note);
      fs.write((uint8_t*)note.c_str(), note.length());
      fs.close();
      SD.end();
      response = "メモを保存しました。";
    }
    else{
      response = "メモを保存できませんでした。";
      SD.end();
    }

  }
  else{
    response = "メモを保存できませんでした。";
  }
  return response;
}

String read_note(){
  String response = "";
  if(note == ""){
    response = "メモはありません";
  }
  else{
    response = "メモの内容は次の通り。" + note;
  }
  return response;
}

String delete_note(){
  String response = "";

  if (SD.begin(GPIO_NUM_4, SPI, 25000000)) {
    auto fs = SD.open("/notepad.txt", FILE_WRITE, true);

    if(fs) {
      note = "";
      fs.write((uint8_t*)note.c_str(), note.length());
      fs.close();
      SD.end();
      response = "メモを消去しました。";
    }
    else{
      response = "メモを消去できませんでした。";
      SD.end();
    }
  }
  else{
    response = "メモを消去できませんでした。";
  }
  return response;
}


String get_bus_time(int nNext){
  String response = "";
  String filename = "";
  int now;
  int nNextCnt = 0;
  struct tm timeInfo; 

  if (getLocalTime(&timeInfo)) {                            // timeinfoに現在時刻を格納
    Serial.printf("現在時刻 %02d:%02d  曜日 %d\n", timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_wday);
    now = timeInfo.tm_hour * 60 + timeInfo.tm_min;
    
    switch(timeInfo.tm_wday){
      case 0:   //日
        filename = "/bus_timetable_holiday.txt";
        break;
      case 1:   //月
      case 2:   //火
      case 3:   //水
      case 4:   //木
      case 5:   //金
        filename = "/bus_timetable.txt";
        break;
      case 6:   //土
        filename = "/bus_timetable_sat.txt";
        break;
    }

    if (SD.begin(GPIO_NUM_4, SPI, 25000000)) {
      auto fs = SD.open(filename.c_str(), FILE_READ);

      if(fs) {
        int hour, min;
        size_t max = 8;
        char buf[max];

        for(int line=0; line<200; line++){
          int len = fs.readBytesUntil(0x0a, (uint8_t*)buf, max);
          if(len == 0){
            Serial.printf("End of file. total line : %d\n", line);
            response = "最後の発車時刻を過ぎました。";
            break;
          }
          else{
            sscanf(buf, "%d:%d", &hour, &min);
            Serial.printf("%03d %02d:%02d\n", line, hour, min);

            int table = hour * 60 + min;
            if(now < table){
              if(nNextCnt == nNext){
                response = String(hour) + "時" + String(min) + "分";
                break;
              }
              else{
                nNextCnt ++;
              }
            }

          }
        }
        fs.close();

      } else {
        Serial.println("Failed to SD.open().");    
        response = "時刻表の読み取りに失敗しました。";  
      }

      SD.end();
    }
    else{
      response = "時刻表の読み取りに失敗しました。";
    }
  }
  else{
    response = "現在時刻の取得に失敗しました。";
  }

  return response;
}


//メッセージをメールで送信する関数
String send_mail(String msg) {
  String response = "";
  EMailSender::EMailMessage message;

  if (authMailAdr != "") {

    EMailSender emailSend(authMailAdr.c_str(), authAppPass.c_str());

    message.subject = "ｽﾀｯｸﾁｬﾝからの通知";
    message.message = msg;
    EMailSender::Response resp = emailSend.send(toMailAdr.c_str(), message);

    if(resp.status == true){
      response = "メール送信成功";
    }
    else{
      response = "メール送信失敗";
    }

  }
  else{
    response = "メールアカウント情報のエラー";
  }


  return response;
}

//受信したメールを読み上げる
String read_mail(void) {
  String response = "";

  if(recvMessages.size() > 0){
    response = String(recvMessages[0]);
    recvMessages.pop_front();
    prev_nMail = recvMessages.size();
  }
  else{
    response = "受信メールはありません。";
  }
  
  return response;
}

// 今日の天気をWeb APIで取得する関数
//   city IDはsetup()でSDカードのファイルから読み込む。
//   city IDの定義はここを見てください。https://weather.tsukumijima.net/primary_area.xml
String weatherUrl = "http://weather.tsukumijima.net/api/forecast/city/";
String weatherCityID = "";

String get_weathers(){
  String payload;
  String response = "";
  DynamicJsonDocument doc(4096);

  if ((WiFi.status() == WL_CONNECTED))
  {
    HTTPClient http;
    //http.begin("http://weather.tsukumijima.net/api/forecast/city/140010");
    http.begin(weatherUrl + weatherCityID);
    int httpCode = http.GET();

    if (httpCode > 0)
    {
      if (httpCode == HTTP_CODE_OK)
      {
        payload = http.getString();
      }
    }
    if (httpCode <= 0)
    {
      Serial.println("Error on HTTP request");
      return "エラーです。";
    }

    deserializeJson(doc, payload);
    String date = doc["publicTimeFormatted"];
    String forecast = doc["description"]["bodyText"];

    Serial.println(date);
    Serial.println(forecast);

    response = forecast;
  }
  return response;
}



String exec_calledFunc(DynamicJsonDocument doc, String* calledFunc){
  String response = "";
  const char* name = doc["choices"][0]["message"]["function_call"]["name"];
  const char* args = doc["choices"][0]["message"]["function_call"]["arguments"];

  Serial.println(name);
  Serial.println(args);

  DynamicJsonDocument argsDoc(1000);
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

    if(strcmp(name, "timer") == 0){
      const int time = argsDoc["time"];
      const char* action = argsDoc["action"];
      Serial.printf("time:%d\n",time);
      Serial.println(action);

      response = timer(time, action);
    }
    else if(strcmp(name, "timer_change") == 0){
      const int time = argsDoc["time"];
      response = timer_change(time);    
    }
    else if(strcmp(name, "get_date") == 0){
      response = get_date();    
    }
    else if(strcmp(name, "get_time") == 0){
      response = get_time();    
    }
    else if(strcmp(name, "get_week") == 0){
      response = get_week();    
    }
    else if(strcmp(name, "listen") == 0){
      response = listen();    
    }
    else if(strcmp(name, "save_note") == 0){
      const char* text = argsDoc["text"];
      Serial.println(text);
      response = save_note(text);
    }
    else if(strcmp(name, "read_note") == 0){
      response = read_note();    
    }
    else if(strcmp(name, "delete_note") == 0){
      response = delete_note();    
    }
    else if(strcmp(name, "get_bus_time") == 0){
      const int nNext = argsDoc["nNext"];
      Serial.printf("nNext:%d\n",nNext);   
      response = get_bus_time(nNext);    
    }
    else if(strcmp(name, "send_mail") == 0){
      const char* text = argsDoc["message"];
      Serial.println(text);
      response = send_mail(text);
    }
    else if(strcmp(name, "read_mail") == 0){
      response = read_mail();    
    }
    else if(strcmp(name, "get_weathers") == 0){
      response = get_weathers();    
    }
  }

  return response;
}
