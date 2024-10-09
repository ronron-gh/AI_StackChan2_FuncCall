#ifndef __STACKCHAN_EX_CONFIG_H__
#define __STACKCHAN_EX_CONFIG_H__

#include <Stackchan_system_config.h>


#if defined(ARDUINO_M5STACK_Core2)
  // #define DEFAULT_SERVO_PIN_X 13  //Core2 PORT C
  // #define DEFAULT_SERVO_PIN_Y 14
  #define DEFAULT_SERVO_PIN_X 33  //Core2 PORT A
  #define DEFAULT_SERVO_PIN_Y 32
#elif defined( ARDUINO_M5STACK_FIRE )
  #define DEFAULT_SERVO_PIN_X 21
  #define DEFAULT_SERVO_PIN_Y 22
#elif defined( ARDUINO_M5Stack_Core_ESP32 )
  #define DEFAULT_SERVO_PIN_X 21
  #define DEFAULT_SERVO_PIN_Y 22
#elif defined( ARDUINO_M5STACK_CORES3 )
  #define DEFAULT_SERVO_PIN_X 18  //CoreS3 PORT C
  #define DEFAULT_SERVO_PIN_Y 17
#endif

//--- Function Call用設定 ---
//Gmailアカウント、アプリパスワード
typedef struct FnMail {
    String account;
    String app_pwd;
    String to_addr;
} mail_s;

//天気予報APIのCity ID
typedef struct FnWeather {
    String city_id;
} weather_s;

//NEWS APIのAPIキー
typedef struct FnNews {
    String apikey;
} news_s;

typedef struct TTSConf {
    int type;
    String model;
    String voice;
} tts_s;


typedef struct ExConfig {
    mail_s mail;
    weather_s weather;
    news_s news;
    tts_s tts;
} ex_config_s;


// StackchanSystemConfigを継承します。
class StackchanExConfig : public StackchanSystemConfig
{
    protected:
        bool USE_SERVO_ST;      //servo.txtの1行目のパラメータの格納先（このソフトでは未使用）。
        ex_config_s _ex_parameters;


    public:
        StackchanExConfig();
        ~StackchanExConfig();

        void loadExtendConfig(fs::FS& fs, const char *yaml_filename, uint32_t yaml_size) override;
        void setExtendSettings(DynamicJsonDocument doc) override;
        void printExtParameters(void) override;

        ex_config_s getExConfig() { return _ex_parameters; }
        void setExConfig(ex_config_s config) { _ex_parameters = config; } 

        void basicConfigNotFoundCallback(void) override;
        void secretConfigNotFoundCallback(void) override;
        void extendConfigNotFoundCallback(void);

};


#endif