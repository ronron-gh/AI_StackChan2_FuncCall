// Copyright (c) Shinya Ishikawa. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#include "SubWindow.h"

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
  subWdWidth = 107;    //縮小率1/3
//  subWdWidth = 160;    //縮小率1/2

//  subWdHeight = 60;    //縮小率1/4
  subWdHeight = 80;    //縮小率1/3
//  subWdHeight = 120;    //縮小率1/2

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
}


void SubWindow::draw(M5Canvas *spi, BoundingRect rect, DrawContext *ctx) {
 
  if(isDrawEnable){
    int x = rect.getLeft();
    int y = rect.getTop();

    spi->startWrite();
    spi->setAddrWindow(x, y, subWdWidth, subWdHeight);
    spi->writePixels((uint16_t*)subWdBuf[drawingBufIdx], subWdSize);
    spi->endWrite();

  }
}

void SubWindow::set_isDrawEnable(bool _isDrawEnable){
  isDrawEnable = _isDrawEnable;
}

void SubWindow::updateBuffer(uint8_t* src){

  uint16_t* resizeBuf;

#if defined(USE_DOUBLE_BUFFER)
  if(drawingBufIdx == 0){
    resizeBuf = subWdBuf[1];
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

}  // namespace m5avatar
