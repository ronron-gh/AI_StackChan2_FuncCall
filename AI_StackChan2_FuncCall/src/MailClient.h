#ifndef _MAIL_CLIENT_H_
#define _MAIL_CLIENT_H_

#include "ESP_Mail_Client.h"
//#include "extras/SDHelper.h"  // Provide the SD card interfaces setting and mounting
#include <deque>

// IMAP
#undef ENABLE_CUSTOM_CLIENT
#define IMAP_HOST "imap.gmail.com"
#define IMAP_PORT 993

#define READ_MAIL_PERIOD (180*1000)     // 3分

extern String authMailAdr;    // "XXXXXXXX@gmail.com"
extern String authAppPass;    // "XXXXXXXXXXXXXXXX"
extern String toMailAdr;      // "XXXXXXXX@XXX.com"

extern TimerHandle_t xTimerReadMail;
extern bool readMailTimerCallbacked;
extern std::deque<String> recvMessages;
extern int prev_nMail;

// 関数プロトタイプ
void imap_init(void);
void imapReadMail(void);
void startReadMailTimer(void);

#endif //_MAIL_CLIENT_H_