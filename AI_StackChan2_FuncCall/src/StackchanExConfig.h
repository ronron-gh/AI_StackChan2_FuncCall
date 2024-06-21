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

// [TODO] Function Call用設定に対応
typedef struct ExConfig {
    String item1;
    int    item2;
    bool   item3;
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
        void setItem1(String item1) { _ex_parameters.item1 = item1; }
        void setItem2(int    item2) { _ex_parameters.item2 = item2; }
        void setItem3(bool   item3) { _ex_parameters.item3 = item3; }
        void setExConfig(ex_config_s config) { _ex_parameters = config; } 

        void basicConfigNotFoundCallback(void) override;
        void secretConfigNotFoundCallback(void) override;

};


#endif