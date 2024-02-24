#ifndef _WAKEWORD_H
#define _WAKEWORD_H

void clear_buff();
void wakeword_setup();
void wakeword_init();
int wakeword_regist();
int wakeword_compare();
extern int mode;   // 0: none, <0: REGIST, >0: COMPARE

#define REGISTER_MAX    (10)
#define DIST_THRESHOLD  (250)

#endif //_WAKEWORD_H