// Copyright (c) Shinya Ishikawa. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#include <Arduino.h>
#include <SD.h>
#include "SubWindow.h"
//#include "chat/ChatGPT/FunctionCall.h"  //APP_DATA_PATHのため

namespace m5avatar {

SubWindow::SubWindow() {
  isDrawEnable = false;
  drawingBufIdx = 0;

  //LCD_WIDTH = 320;
  //LCD_HEIGHT = 240;
  //RESIZE_RATIO = 3;

  LCD_WIDTH = 640;
  LCD_HEIGHT = 480;
  RESIZE_RATIO = 6;

//  subWdWidth = 80;    //縮小率1/4
//  subWdWidth = 107;    //縮小率1/3
//  subWdWidth = 160;    //縮小率1/2
  subWdWidth = 320;    //縮小率1/1

//  subWdHeight = 60;    //縮小率1/4
//  subWdHeight = 80;    //縮小率1/3
//  subWdHeight = 120;    //縮小率1/2
  subWdHeight = 240;    //縮小率1/1

  subWdSize = subWdWidth * subWdHeight;

  subWdBuf[0] = (uint16_t*)malloc(subWdSize * 2);
  if(subWdBuf[0]==NULL){
    Serial.println("SubWindow: malloc() failed");    
  }
#if defined(USE_DOUBLE_BUFFER)
  subWdBuf[1] = (uint16_t*)malloc(subWdSize * 2);
  if(subWdBuf[1]==NULL){
    Serial.println("SubWindow: malloc() failed");    
  }
#endif

  //テキスト表示用のスプライト
  spriteTxt = new M5Canvas(&M5.Lcd);
  if(spriteTxt == nullptr){
    M5_LOGE("spriteTxt new error");
  }
}


void SubWindow::draw(M5Canvas *spi, BoundingRect rect, DrawContext *ctx) {
 
  if(isDrawEnable){
    if(drawType == SUB_DRAW_TYPE_IMG){
      int x = rect.getLeft();
      int y = rect.getTop();
      spi->startWrite();
      spi->setAddrWindow(x, y, subWdWidth, subWdHeight);
      spi->writePixels((uint16_t*)subWdBuf[drawingBufIdx], subWdSize);
      spi->endWrite();
    }
    else if(drawType == SUB_DRAW_TYPE_JPG){
      #if 0
      spriteTxt->setColorDepth(ctx->getColorDepth());
      spriteTxt->createSprite(subWdWidth, subWdHeight);
      spriteTxt->setBitmapColor(ctx->getColorPalette()->get(COLOR_PRIMARY),
        ctx->getColorPalette()->get(COLOR_BACKGROUND));
      //spriteTxt->fillSprite(GREENYELLOW);
      spriteTxt->fillRect(0, 0, subWdWidth, subWdHeight, GREENYELLOW);
      //spriteTxt->drawJpg(jpgBuf, jpgSize, 0, 0);
      spriteTxt->pushSprite(spi, 0, 0);
      spriteTxt->deleteSprite();
      #endif
      //spi->fillRect(0, 0, subWdWidth, subWdHeight, GREENYELLOW);
      //spi->drawJpg(jpgBuf, jpgSize, 0, 0, subWdWidth, subWdHeight);
      
#if 1      
      if (SD.begin(GPIO_NUM_4, SPI, 25000000)) {
        //String fname = String(APP_DATA_PATH) + "photo/photo001.jpg";
        spi->drawJpgFile(SD, "/app/AiStackChan2FuncCall/photo/photo001.jpg", 0, 0, subWdWidth, subWdHeight);
      }
      else{
        Serial.println("Failed to load SD card settings. System reset after 5 seconds.");
      }
#endif
    }
    else if(drawType == SUB_DRAW_TYPE_TXT){
      int x = rect.getLeft();
      int y = rect.getTop();
      int w = rect.getWidth();
      int h = rect.getHeight();
      spriteTxt->setColorDepth(ctx->getColorDepth());
      spriteTxt->createSprite(M5.Display.width(), M5.Display.height());
      spriteTxt->setBitmapColor(ctx->getColorPalette()->get(COLOR_PRIMARY),
        ctx->getColorPalette()->get(COLOR_BACKGROUND));
      if (ctx->getColorDepth() != 1) {
        spriteTxt->fillSprite(ctx->getColorPalette()->get(COLOR_BACKGROUND));
      } else {
        spriteTxt->fillSprite(0);
      }
      //spi->setClipRect(x, y, w, h);
      spriteTxt->fillRect(0, 0, w, h, BLACK);
      spriteTxt->setFont(&fonts::lgfxJapanGothic_20);
      spriteTxt->setTextSize(1);
      spriteTxt->setTextColor(WHITE, BLACK);
      spriteTxt->setTextDatum(0);
      spriteTxt->setCursor(0, 0);
      spriteTxt->print(subWdTxtBuf);
      spriteTxt->pushSprite(spi, x, y);
      spriteTxt->deleteSprite();
    }
  }
}

void SubWindow::set_isDrawEnable(bool _isDrawEnable){
  isDrawEnable = _isDrawEnable;
}

void SubWindow::updateDrawContentImg(uint8_t* src){

  uint16_t* resizeBuf;

  drawType = SUB_DRAW_TYPE_IMG;

#if defined(USE_DOUBLE_BUFFER)
  if(drawingBufIdx == 0){
    resizeBuf = subWdBuf[1];
  }
  else{
    resizeBuf = subWdBuf[0];
  }
#else
  resizeBuf = subWdBuf[0];
#endif

  //画像を縮小
  uint16_t* buf16 = (uint16_t*)src;
  int idx=0;
  for(int y=0; y<LCD_HEIGHT; y++){
    if(y%RESIZE_RATIO != 0) continue;
    for(int x=0; x<LCD_WIDTH; x++){
      if(x%RESIZE_RATIO != 0) continue;

      resizeBuf[idx++] = buf16[y*LCD_WIDTH + x];

    }

  }

#if defined(USE_DOUBLE_BUFFER)
  //表示するバッファを切り替え
  if(drawingBufIdx == 0){
    drawingBufIdx = 1;
  }
  else{
    drawingBufIdx = 0;
  }
#endif
}


void SubWindow::updateDrawContentJpg(uint8_t* src, int32_t size){
  drawType = SUB_DRAW_TYPE_JPG;
  jpgBuf = new uint8_t[size];
  jpgSize = size;
  memcpy(jpgBuf, src, size);
}

void SubWindow::updateDrawContentTxt(String txt){
  drawType = SUB_DRAW_TYPE_TXT;
  subWdTxtBuf = txt;
}

}  // namespace m5avatar
