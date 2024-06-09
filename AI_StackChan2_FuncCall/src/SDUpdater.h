#ifndef _SD_UPDATER_H
#define _SD_UPDATER_H

#if defined( ENABLE_SD_UPDATER )

extern bool USE_SERVO_ST;
extern int SERVO_PIN_X;
extern int SERVO_PIN_Y;

extern void SDU_lobby(String PROG_NAME);
extern bool servoTxtSDRead();

#endif  // ENABLE_SD_UPDATER

#endif  // _SD_UPDATER_H