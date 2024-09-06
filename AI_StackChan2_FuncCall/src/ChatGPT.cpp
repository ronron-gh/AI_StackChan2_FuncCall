#include <Arduino.h>
#include <M5Unified.h>
#include <SPIFFS.h>
#include <Avatar.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
//#include "rootCACertificate.h"
#include <ArduinoJson.h>
#include "SpiRamJsonDocument.h"
#include "ChatHistory.h"
#include "FunctionCall.h"
#include "HexLED.h"
#include "Speech.h"

using namespace m5avatar;
extern Avatar avatar;
extern String speech_text;
extern String speech_text_buffer;
extern const char* root_ca_openai;

String OPENAI_API_KEY = "";

// 保存する質問と回答の最大数
//const int MAX_HISTORY = 5;
const int MAX_HISTORY = 20;

// 過去の質問と回答を保存するデータ構造
ChatHistory chatHistory(MAX_HISTORY);

//DynamicJsonDocument chat_doc(1024*10);
//DynamicJsonDocument chat_doc(1024*30);
SpiRamJsonDocument chat_doc(0);     // PSRAMから確保するように変更。サイズの確保はsetup()で実施（初期化後でないとPSRAMが使えないため）。

// ChatGPTのJSONのテンプレートはFunctionCall.cppで定義
//String json_ChatString = "{\"model\": \"gpt-3.5-turbo-0613\",\"messages\": [{\"role\": \"user\", \"content\": \"""\"}]}";
//String Role_JSON = "";

String InitBuffer = "";

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
#if defined( ENABLE_HEX_LED )  
  hex_led_ptn_thinking_start();
#endif
  String ret = https_post_json("https://api.openai.com/v1/chat/completions", json_string.c_str(), root_ca_openai);
  avatar.setExpression(Expression::Neutral);
  avatar.setSpeechText("");
#if defined( ENABLE_HEX_LED )
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


#define MAX_REQUEST_COUNT  (10)
void exec_chatGPT(String text, const char *base64_buf = NULL) {
  static String response = "";
  String calledFunc = "";
  //String funcCallMode = "auto";
  bool image_flag = false;

  //Serial.println(InitBuffer);
  //init_chat_doc(InitBuffer.c_str());

  // 質問をチャット履歴に追加
  if(base64_buf == NULL){
    chatHistory.push_back(String("user"), String(""), text);
  }
  else{
    //画像が入力された場合は第2引数を"image"にして識別する
    chatHistory.push_back(String("user"), String("image"), text);
  }

  // functionの実行が要求されなくなるまで繰り返す
  for (int reqCount = 0; reqCount < MAX_REQUEST_COUNT; reqCount++)
  {
    init_chat_doc(InitBuffer.c_str());

    //if(reqCount == (MAX_REQUEST_COUNT - 1)){
    //  funcCallMode = String("none");
    //}

    for (int i = 0; i < chatHistory.get_size(); i++)
    {
      JsonArray messages = chat_doc["messages"];
      JsonObject systemMessage1 = messages.createNestedObject();

      if(chatHistory.get_role(i).equals(String("function"))){
        //Function Callingの場合
        systemMessage1["role"] = chatHistory.get_role(i);
        systemMessage1["name"] = chatHistory.get_funcName(i);
        systemMessage1["content"] = chatHistory.get_content(i);
      }
      else if(chatHistory.get_funcName(i).equals(String("image"))){
        //画像がある場合
        //このようなJSONを作成する
        // messages=[
        //      {"role": "user", "content": [
        //          {"type": "text", "text": "この三角形の面積は？"},
        //          {"type": "image_url", "image_url": {"url": f"data:image/png;base64,{base64_image}"}}
        //      ]}
        //  ],

        String image_url_str = String("data:image/jpeg;base64,") + String(base64_buf); 

        systemMessage1["role"] = chatHistory.get_role(i);
        JsonObject content_text = systemMessage1["content"].createNestedObject();
        content_text["type"] = "text";
        content_text["text"] = chatHistory.get_content(i);
        JsonObject content_image = systemMessage1["content"].createNestedObject();
        content_image["type"] = "image_url";
        content_image["image_url"]["url"] = image_url_str.c_str();

        //次回以降は画像の埋め込みをしないよう、識別用の文字列"image"を消す
        chatHistory.set_funcName(i, "");
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


    if(calledFunc == ""){   // Function Callなし ／ Function Call繰り返しの完了
      chatHistory.push_back(String("assistant"), String(""), response);   // 返答をチャット履歴に追加
      speech(response);
      break;
    }
    else{   // Function Call繰り返し中。ループを継続
      chatHistory.push_back(String("function"), calledFunc, response);   // 返答をチャット履歴に追加   
    }

  }

  //チャット履歴の容量を圧迫しないように、functionロールを削除する
  chatHistory.clean_function_role();
}
