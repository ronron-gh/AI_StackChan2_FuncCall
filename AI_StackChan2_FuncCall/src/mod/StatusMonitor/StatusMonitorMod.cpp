#include <Arduino.h>
#include "mod/ModManager.h"
#include "StatusMonitorMod.h"
#include "Robot.h"
#include "chat/ChatGPT/ChatGPT.h"
#include <WiFiClientSecure.h>


/// 外部参照 ///
extern void sw_tone();

///////////////


static void init_info_disp()
{
  //M5.Display.setTextFont(0);
  M5.Display.setFont(&fonts::lgfxJapanGothic_20);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(WHITE, BLACK);
  M5.Display.setTextDatum(0);
  M5.Display.setCursor(0, 0);
  M5.Display.fillScreen(BLACK);
  
  delay(100);
}

StatusMonitorMod::StatusMonitorMod(void)
{
  box_BtnA.setupBox(0, 100, 40, 60);
  box_BtnC.setupBox(280, 100, 40, 60);
}

void StatusMonitorMod::init(void)
{
  update(0);
}

void StatusMonitorMod::update(int page_no)
{
  String str = "";
  char tmp[256];

  init_info_disp();

  str += "======== System status ========\n";
  sprintf(tmp, "Wifi:\n  IP addr:%s\n",
                  WiFi.localIP().toString().c_str() );
  str += tmp;
  sprintf(tmp, "Heap largest free block:\n  DMA:%d\n  SPIRAM:%d\n",
                  heap_caps_get_largest_free_block(MALLOC_CAP_DMA),
                  heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) );
  str += tmp;
  sprintf(tmp, "Prompt buffer usage:\n  %d / %d [byte]\n",
                  chat_doc.memoryUsage(),
                  chat_doc.capacity() );
  str += tmp;
  sprintf(tmp, "Battery level:  %d %%\n", M5.Power.getBatteryLevel());
  str += tmp;
  M5.Display.print(str);
}

void StatusMonitorMod::display_touched(int16_t x, int16_t y)
{

  if (box_BtnA.contain(x, y))
  {

  }

  if (box_BtnC.contain(x, y))
  {
    //sw_tone();
    change_mod();
  }

}

