#include "Robot.h"
#include "tts/WebVoiceVoxTTS.h"
#include "tts/ElevenLabsTTS.h"
#include "tts/OpenAITTS.h"
#include "stt/CloudSpeechClient.h"
#include "stt/Whisper.h"
#include "driver/Audio.h"
#include "driver/AudioWhisper.h"
#include "rootCA/rootCAgoogle.h"
#include "rootCA/rootCACertificate.h"
#include "Avatar.h"

using namespace m5avatar;

extern Avatar avatar;
extern bool servo_home;
extern String OPENAI_API_KEY;
extern String STT_API_KEY;

Robot::Robot(StackchanExConfig& config)
{

    //TTS
    tts_param_t param;
    api_keys_s* api_key = config.getAPISetting();
    param.api_key = api_key->tts;
    param.model = config.getExConfig().tts.model;
    param.voice = config.getExConfig().tts.voice;
    int type = config.getExConfig().tts.type;
    switch(type){
    case 0:
        tts = new WebVoiceVoxTTS(param);
        break;
    case 1:
        tts = new ElevenLabsTTS(param);
        break;
    case 2:
        param.api_key = api_key->ai_service;    //API KeyはChatGPTと共通
        tts = new OpenAITTS(param);
        break;
    }

}


void Robot::speech(String text)
{
    servo_home = false;
    avatar.setExpression(Expression::Happy);
    
    tts->stream(text);

    avatar.setExpression(Expression::Neutral);
    servo_home = true;
}

String Robot::listen(bool isGoogle)
{
  //TODO ttsと同じようにsttをクラス実装する

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

