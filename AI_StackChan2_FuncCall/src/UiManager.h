#ifndef _UI_MANAGER_H
#define _UI_MANAGER_H

#define MAX_UI_NUM  (10)


struct box_t
{
  int x;
  int y;
  int w;
  int h;
  int touch_id = -1;

  void setupBox(int x, int y, int w, int h) {
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
  }
  bool contain(int x, int y)
  {
    return this->x <= x && x < (this->x + this->w)
        && this->y <= y && y < (this->y + this->h);
  }
};


class UiBase{
protected:

public:
    UiBase(void);
    virtual void init(void);
    virtual void update(int page_no);
    virtual void btnA_pressed(void);
    virtual void btnA_longPressed(void);
    virtual void btnB_pressed(void);
    virtual void btnB_longPressed(void);
    virtual void btnC_pressed(void);
    virtual void btnC_longPressed(void);
    virtual void display_touched(int16_t x, int16_t y);
    virtual void idle(void);
};



class UiAvatar: public UiBase{
private:
    box_t box_servo;
    box_t box_stt;
    box_t box_BtnA;
    box_t box_BtnC;
    #if defined(ENABLE_FACE_DETECT)
    box_t box_subWindow;
    #endif
    String avatarText;
public:
    UiAvatar(void);
    void init(void);
    void update(int page_no);
    void btnA_pressed(void);
    void btnB_longPressed(void);
    void btnC_pressed(void);
    void display_touched(int16_t x, int16_t y);
    void idle(void);
};


class UiStatus: public UiBase{
private:
    box_t box_BtnA;
    box_t box_BtnC;

public:
    UiStatus(void);
    void init(void);
    void update(int page_no);
    void display_touched(int16_t x, int16_t y);
};


class UiFuncCallInfo: public UiBase{
private:
    box_t box_BtnA;
    box_t box_BtnC;
    box_t box_BtnUA;
    box_t box_BtnUC;
    const int MAX_PAGE_NO = 2;
    int current_page_no;
public:
    UiFuncCallInfo(void);
    void init(void);
    void update(int page_no);
    void display_touched(int16_t x, int16_t y);
};



extern UiBase* init_ui(void);
extern UiBase* get_current_ui(void);

#endif