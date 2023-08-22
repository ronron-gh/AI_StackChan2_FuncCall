#ifndef _HEX_LED_H_
#define _HEX_LED_H_

#include <FastLED.h>

#define NUM_LEDS 37
//#define LED_DATA_PIN 9    //PORTB
#define LED_DATA_PIN 2    //PORTA
#define LED_BRIGHTNESS 8

void hex_led_init(void);
void hex_led_ptn_off(void);
void hex_led_ptn_wake(void);
void hex_led_ptn_accept(void);
void hex_led_ptn_notification(void);
void hex_led_ptn_boot(void);
void hex_led_ptn_thinking_start(void);
void hex_led_ptn_thinking_end(void);

#endif //_HEX_LED_H_