#pragma once

#ifndef RFC2047_H
#define RFC2047_H

#include "ESP_Mail_Client_Version.h"
#if !VALID_VERSION_CHECK(30307)
#error "Mixed versions compilation."
#endif

#include <Arduino.h>
#include "ESP_Mail_FS.h"
#include "MB_FS.h"

#if defined(ESP32)
#if defined(BOARD_HAS_PSRAM) && defined(ESP_Mail_USE_PSRAM)
#include <esp32-hal-psram.h>
#endif
#endif

#define strfcpy(A, B, C) strncpy(A, B, C), *(A + (C)-1) = 0

enum
{
    ENCOTHER,
    ENC7BIT,
    ENC8BIT,
    ENCQUOTEDPRINTABLE,
    ENCBASE64,
    ENCBINARY
};

__attribute__((used)) static const char *Charset = "utf-8";

__attribute__((used)) static int Index_hex[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

__attribute__((used)) static int Index_64[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1};

#define IsPrint(c) (isprint((unsigned char)(c)) || \
                    ((unsigned char)(c) >= 0xa0))

#define hexval(c) Index_hex[(unsigned int)(c)]
#define base64val(c) Index_64[(unsigned int)(c)]

class RFC2047_Decoder
{

public:
    RFC2047_Decoder();
    ~RFC2047_Decoder();
    void decode(MB_FS *mbfs, char *d, const char *s, size_t dlen);

private:
    void rfc2047DecodeWord(char *d, const char *s, size_t dlen);
    char *safe_strdup(const char *s);
    MB_FS *mbfs = nullptr;
};

#endif // RFC2047_H