#include <Arduino.h>
#include <M5Unified.h>
#include <SD.h>
#include <SPIFFS.h>
#include <AudioOutput.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include "AudioFileSourceHTTPSStream.h"
//#include "AudioFileSourceSD.h"
#include "AudioFileSourceSPIFFS.h"
#include "AudioOutputM5Speaker.h"
#include "PlayMP3.h"
#include "Avatar.h"

using namespace m5avatar;

extern Avatar avatar;
extern bool servo_home;

/// set M5Speaker virtual channel (0-7)
//static constexpr uint8_t m5spk_virtual_channel = 0;
uint8_t m5spk_virtual_channel = 0;

AudioOutputM5Speaker out(&M5.Speaker, m5spk_virtual_channel);
AudioGeneratorMP3 *mp3;

int preallocateBufferSize = 30*1024;
uint8_t *preallocateBuffer;




void mp3_init(void)
{
    mp3 = new AudioGeneratorMP3();
    //out = new AudioOutputM5Speaker(&M5.Speaker, m5spk_virtual_channel);

    //TTS MP3用バッファ （PSRAMから確保される）
    preallocateBuffer = (uint8_t *)malloc(preallocateBufferSize);
    if (!preallocateBuffer) {
        M5.Display.printf("FATAL ERROR:  Unable to preallocate %d bytes for app\n", preallocateBufferSize);
        for (;;) { delay(1000); }
    }
}

void playMP3(AudioFileSourceBuffer *buff){


  M5.Mic.end();
  M5.Speaker.begin();

  mp3->begin(buff, &out);
  
  while(mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
      Serial.println("mp3 stop");
    }
    delay(1);
  }

  delay(200);
  M5.Speaker.end();
  M5.Mic.begin();

}

bool playMP3File(const char *filename)
{
  bool result;

  if (SPIFFS.exists(filename)) {

    //AudioFileSourceSD *file_mp3 = new AudioFileSourceSD(filename);
    AudioFileSourceSPIFFS *file_mp3 = new AudioFileSourceSPIFFS(filename);
    Serial.println("Open mp3");
    
    if( !file_mp3->isOpen() ){
      delete file_mp3;
      file_mp3 = nullptr;
      Serial.println("failed to open mp3 file");
      result = false;
    }
    else{

/////// AudioFileSourceBufferを経由してPlayMP3()を呼べばよいかも //////////////      
      avatar.setExpression(Expression::Happy);
      M5.Mic.end();
      M5.Speaker.begin();
  #if defined(ENABLE_WAKEWORD)
      //mode = 0;
  #endif
      servo_home = false;
      mp3->begin(file_mp3, &out);

      while(mp3->isRunning()) {
        if (!mp3->loop()) {
          mp3->stop();
          //if(file != nullptr){delete file; file = nullptr;}
          //if(buff != nullptr){delete buff; buff = nullptr;}
          Serial.println("mp3 stop");
          avatar.setExpression(Expression::Neutral);
          delay(200);
          M5.Speaker.end();
          M5.Mic.begin();

          delete file_mp3;
          file_mp3 = nullptr;
        }
        delay(1);
      }
      servo_home = true;
/////////////////////////////////

      result = true;

    }
  }else{
    Serial.println("mp3 file is not exist");
    result = false;
  }
  return result;
}
