#ifndef _STATUS_MONITOR_MOD_H
#define _STATUS_MONITOR_MOD_H

#include <arduino.h>
#include "mod/ModBase.h"


class StatusMonitorMod: public ModBase{
private:
    box_t box_BtnA;
    box_t box_BtnC;

public:
    StatusMonitorMod(void);
    void init(void);
    void update(int page_no);
    void display_touched(int16_t x, int16_t y);
};

#endif  //_STATUS_MONITOR_MOD_H