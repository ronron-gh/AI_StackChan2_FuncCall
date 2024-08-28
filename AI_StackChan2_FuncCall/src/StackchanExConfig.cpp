#include "StackchanExConfig.h"
#include "SDUtil.h"

StackchanExConfig::StackchanExConfig() {};
StackchanExConfig::~StackchanExConfig() {};


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

void StackchanExConfig::loadExtendConfig(fs::FS& fs, const char* yaml_filename, uint32_t yaml_size)
{
    M5_LOGI("----- StackchanExConfig::loadExtendConfig:%s\n", yaml_filename);
    File file = fs.open(yaml_filename);
    if (file) {
        DynamicJsonDocument doc(yaml_size);
        auto err = deserializeYml( doc, file);
        if (err) {
            M5_LOGE("yaml file read error: %s\n", yaml_filename);
            M5_LOGE("error%s\n", err.c_str());
            extendConfigNotFoundCallback();
        }
        else{
            setExtendSettings(doc);
        }

        serializeJsonPretty(doc, Serial);
        M5_LOGI("");
        printExtParameters();

    }
    else{
        //YAMLファイルオープン失敗の場合は、SDのディレクトリ直下に旧TXTファイルがあれば読み込む
        M5_LOGE("yaml file open error: %s\n", yaml_filename);
        extendConfigNotFoundCallback();
        printExtParameters();
    }
}

void StackchanExConfig::extendConfigNotFoundCallback(void)
{
    M5_LOGE("load extend config from txt files\n");

    /// News API key
    {
        char buf[128], key[128];
        if(read_sd_file("/news_api_key.txt", buf, sizeof(buf))){
            read_line_from_buf(buf, key);
            _ex_parameters.news.apikey = String(key);
        }
    }

    /// Weather city ID
    {
        char buf[128], key[128];
        if(read_sd_file("/weather_city_id.txt", buf, sizeof(buf))){
            read_line_from_buf(buf, key);
            _ex_parameters.weather.city_id = String(key);
        }
        else{
            _ex_parameters.weather.city_id = "130010";
        }
    }

    /// Gmail
    {
        char buf[256], key[256];
        if(read_sd_file("/gmail.txt", buf, sizeof(buf))){
            read_line_from_buf(buf, key);
            _ex_parameters.mail.account = String(key);

            read_line_from_buf(nullptr, key);
            _ex_parameters.mail.app_pwd = String(key);

            read_line_from_buf(nullptr, key);
            _ex_parameters.mail.to_addr = String(key);
        }
    }
}

void StackchanExConfig::setExtendSettings(DynamicJsonDocument doc)
{

    _ex_parameters.mail.account     = doc["mail"]["account"].as<String>();
    _ex_parameters.mail.app_pwd     = doc["mail"]["app_pwd"].as<String>();
    _ex_parameters.mail.to_addr     = doc["mail"]["to_addr"].as<String>();
    _ex_parameters.weather.city_id  = doc["weather"]["city_id"].as<String>();
    _ex_parameters.news.apikey      = doc["news"]["apikey"].as<String>();

}

void StackchanExConfig::printExtParameters(void)
{
    M5_LOGI("mail account: %s", _ex_parameters.mail.account.c_str());
    M5_LOGI("mail app password: %s", _ex_parameters.mail.app_pwd.c_str());
    M5_LOGI("mail to addr: %s", _ex_parameters.mail.to_addr.c_str());
    M5_LOGI("weather city id: %s", _ex_parameters.weather.city_id.c_str());
    M5_LOGI("news apikey: %s", _ex_parameters.news.apikey.c_str());
}
