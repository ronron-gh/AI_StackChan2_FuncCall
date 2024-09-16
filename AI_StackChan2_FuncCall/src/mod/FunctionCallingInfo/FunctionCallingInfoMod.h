#ifndef _FUNC_CALL_INFO_MOD_H
#define _FUNC_CALL_INFO_MOD_H

#include <arduino.h>
#include "mod/ModBase.h"


class FunctionCallingInfoMod: public ModBase{
private:
    box_t box_BtnA;
    box_t box_BtnC;
    box_t box_BtnUA;
    box_t box_BtnUC;
    const int MAX_PAGE_NO = 2;
    int current_page_no;
public:
    FunctionCallingInfoMod(void);
    void init(void);
    void update(int page_no);
    void display_touched(int16_t x, int16_t y);
};

#endif  //_FUNC_CALL_INFO_MOD_H