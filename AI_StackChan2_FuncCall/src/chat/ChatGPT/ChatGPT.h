#ifndef _CHAT_GPT_H
#define _CHAT_GPT_H

#include <Arduino.h>
#include <M5Unified.h>
#include "SpiRamJsonDocument.h"
#include "ChatHistory.h"


extern String OPENAI_API_KEY;
extern String InitBuffer;
extern SpiRamJsonDocument chat_doc;
extern ChatHistory chatHistory;

extern bool init_chat_doc(const char *data);
extern bool save_json();
extern void exec_chatGPT(String text, const char *base64_buf = NULL);


#endif  //_CHAT_GPT_H