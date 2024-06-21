#include "StackchanExConfig.h"
#include "SDUtil.h"

StackchanExConfig::StackchanExConfig() {};
StackchanExConfig::~StackchanExConfig() {};

void StackchanExConfig::loadExtendConfig(fs::FS& fs, const char *yaml_filename, uint32_t yaml_size) {};
void StackchanExConfig::setExtendSettings(DynamicJsonDocument doc) {};
void StackchanExConfig::printExtParameters(void) {};

void StackchanExConfig::basicConfigNotFoundCallback(void)
{
    char buf[128], data[128];
    char *endp;
    String SV_ON_OFF = "";

    Serial.printf("Cannot open YAML basic config file. Try to read legacy text file.\n");

    /// Servo
    if(read_sd_file("/servo.txt", buf, sizeof(buf))){
        read_line_from_buf(buf, data);
        SV_ON_OFF = String(data);
        if (SV_ON_OFF == "on" || SV_ON_OFF == "ON"){
            USE_SERVO_ST = true;
        }
        else{
            USE_SERVO_ST = false;
        }
        Serial.printf("Servo ON or OFF: %s\n",data);

        read_line_from_buf(nullptr, data);
        _servo[AXIS_X].pin = strtol(data, &endp, 10);
        Serial.printf("Servo pin X: %d\n", _servo[AXIS_X].pin);

        read_line_from_buf(nullptr, data);
        _servo[AXIS_Y].pin = strtol(data, &endp, 10);
        Serial.printf("Servo pin Y: %d\n", _servo[AXIS_Y].pin);
    }
    else{
        Serial.printf("Cannot open legacy text file. Set default value.\n");
        _servo[AXIS_X].pin = DEFAULT_SERVO_PIN_X;
        _servo[AXIS_Y].pin = DEFAULT_SERVO_PIN_Y;
        Serial.printf("Servo pin X: %d\n", _servo[AXIS_X].pin);
        Serial.printf("Servo pin Y: %d\n", _servo[AXIS_Y].pin);
    }
}

void StackchanExConfig::secretConfigNotFoundCallback(void)
{
    char buf[128], data[128];

    Serial.printf("Cannot open YAML secret config file. Try to read legacy text file.\n");

    /// wifi
    if(read_sd_file("/wifi.txt", buf, sizeof(buf))){
        read_line_from_buf(buf, data);
        _secret_config.wifi_info.ssid = String(data);
        Serial.printf("SSID: %s\n",data);

        read_line_from_buf(nullptr, data);
        _secret_config.wifi_info.password = String(data);
        Serial.printf("Key: %s\n",data);
    }

    /// apikey
    if(read_sd_file("/apikey.txt", buf, sizeof(buf))){
        read_line_from_buf(buf, data);
        _secret_config.api_key.ai_service = String(data);
        Serial.printf("openai: %s\n",data);

        read_line_from_buf(nullptr, data);
        _secret_config.api_key.tts = String(data);
        Serial.printf("voicevox: %s\n",data);

        read_line_from_buf(nullptr, data);
        _secret_config.api_key.stt = String(data);
        Serial.printf("stt: %s\n",data);

    }
}
