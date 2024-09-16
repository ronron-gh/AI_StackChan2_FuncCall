#include <Arduino.h>
#include "mod/ModManager.h"
#include "FunctionCallingInfoMod.h"
#include "Robot.h"
#include "Scheduler.h"
#include "MySchedule.h"
#include "chat/ChatGPT/ChatGPT.h"
#include "chat/ChatGPT/FunctionCall.h"


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


FunctionCallingInfoMod::FunctionCallingInfoMod(void)
{
  box_BtnA.setupBox(0, 100, 40, 60);
  box_BtnC.setupBox(280, 100, 40, 60);
  box_BtnUA.setupBox(0, 0, 80, 60);
  box_BtnUC.setupBox(240, 0, 80, 60);

  current_page_no = 0;
}

void FunctionCallingInfoMod::init(void)
{
  update(current_page_no);
}

void FunctionCallingInfoMod::update(int page_no)
{
  String str = "";
  char tmp[256];

  init_info_disp();

  str += "<- prev                 next ->\n";
  str += "===== Function call info  =====\n";
  if(page_no == 0){
    str += "Reminder:\n";
    for(int i=0; i<MAX_SCHED_NUM; i++){
      ScheduleBase *sched = get_schedule(i);
      if(sched != nullptr){
          if (sched->get_sched_type() == SCHED_REMINDER) {  
              ScheduleReminder *sched_rem = (ScheduleReminder*)sched;
              str += " " + sched_rem->get_time_string() + " " + sched_rem->get_remind_string() + "\n";
          }
      }
    }
  }
  else if(page_no == 1){
    str += "Memo:\n";
    str += note;
  }

  M5.Display.print(str);

}

void FunctionCallingInfoMod::display_touched(int16_t x, int16_t y)
{

  if (box_BtnA.contain(x, y))
  {

  }

  if (box_BtnC.contain(x, y))
  {
    //sw_tone();
    change_mod();
  }

  if (box_BtnUA.contain(x, y))
  {
    current_page_no--;
    if(current_page_no < 0) current_page_no = MAX_PAGE_NO - 1;
    update(current_page_no);
  }

  if (box_BtnUC.contain(x, y))
  {
    current_page_no++;
    if(current_page_no >= MAX_PAGE_NO) current_page_no = 0;
    update(current_page_no);
  }

}
