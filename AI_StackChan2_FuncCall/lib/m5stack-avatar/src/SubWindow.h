// Copyright (c) Shinya Ishikawa. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#ifndef SUBWINDOW_H_
#define SUBWINDOW_H_

#define LGFX_USE_V1
#include <M5GFX.h>
#include "DrawContext.h"
#include "Drawable.h"

//#define USE_DOUBLE_BUFFER

namespace m5avatar {

class SubWindow final : public Drawable {
 private:
  uint16_t r;
  bool isLeft;

  int32_t RESIZE_RATIO, LCD_WIDTH, LCD_HEIGHT;
  uint32_t subWdWidth, subWdHeight, subWdSize;
  uint16_t* subWdBuf[2];
  uint32_t drawingBufIdx;
  bool isDrawEnable;

 public:
  // constructor
  SubWindow();
  ~SubWindow() = default;
  //SubWindow(const SubWindow &other) = default;
  //SubWindow &operator=(const SubWindow &other) = default;
  void draw(M5Canvas *spi, BoundingRect rect,
            DrawContext *drawContext) override;

  void set_isDrawEnable(bool _isDrawEnable);
  void updateBuffer(uint8_t* buf);
};

}  // namespace m5avatar

#endif  // SUBWINDOW_H_
