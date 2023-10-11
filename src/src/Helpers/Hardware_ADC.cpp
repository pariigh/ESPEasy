#include "../Helpers/Hardware_ADC.h"


#ifdef ESP32
# include "../Helpers/Hardware.h"
#endif // ifdef ESP32

#ifdef ESP8266
bool Hardware_ADC_t::init() {}


int  Hardware_ADC_t::read() {
  if (!WiFiEventData.wifiConnectInProgress) {
# if FEATURE_ADC_VCC
    _lastADCvalue = ESP.getVcc();
# else // if FEATURE_ADC_VCC
    _lastADCvalue = analogRead(A0);
# endif // if FEATURE_ADC_VCC
  }
  return _lastADCvalue;
}

#endif // ifdef ESP8266


#ifdef ESP32
Hardware_ADC_t::~Hardware_ADC_t()
{
# if ESP_IDF_VERSION_MAJOR >= 5
  adc_oneshot_del_unit(_adc_handle);
# else
  // FIXME TD-er: Should we reset the GPIO config for this pin?
# endif // if ESP_IDF_VERSION_MAJOR >= 5
}

bool Hardware_ADC_t::init(int pin, adc_atten_t attenuation)
{
  int ch{};
  int t = -1;

  if (!getADC_gpio_info(pin, _adc, ch, t)) {
    return false;
  }
  _pin = pin;
# if HAS_TOUCH_GPIO
  _isTouchPin = t >= 0;
# endif // if HAS_TOUCH_GPIO
  _attenuation = attenuation;

# if ESP_IDF_VERSION_MAJOR >= 5
  _channel = static_cast<adc_channel_t>(ch);
#  if HAS_ADC2
  const adc_unit_t unit = (_adc == 1) ? ADC_UNIT_1 : ADC_UNIT_2;
#  else // if HAS_ADC2

  if (_adc != 1) {
    return false;
  }
  const adc_unit_t unit = ADC_UNIT_1;
#  endif // if HAS_ADC2

  adc_oneshot_unit_init_cfg_t init_config = {
    .unit_id = unit
  };

  if (ESP_OK != adc_oneshot_new_unit(&init_config, &_adc_handle)) {
    return false;
  }

  adc_oneshot_chan_cfg_t config = {
    .atten    = attenuation,
    .bitwidth = ADC_BITWIDTH_DEFAULT,
  };
  return ESP_OK == adc_oneshot_config_channel(_adc_handle, _channel, &config);

# else // if ESP_IDF_VERSION_MAJOR >= 5

  if ((_adc == 1) || (_adc == 2)) {
    analogSetPinAttenuation(_pin, static_cast<adc_attenuation_t>(_attenuation));
  }

  return true;
# endif // if ESP_IDF_VERSION_MAJOR >= 5
}

bool Hardware_ADC_t::adc_calibration_init()
{
  _useFactoryCalibration = _adc_cali_handle.init(_pin, _attenuation);
  return _useFactoryCalibration;
}

int Hardware_ADC_t::read(bool readAsTouch) {
# if HAS_HALL_EFFECT_SENSOR

  if (_adc == 0) {
    return hallRead();
  }
# endif // if HAS_HALL_EFFECT_SENSOR

  // See:
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html#configuration-and-reading-adc
  // ADC2 is shared with WiFi, so don't read ADC2 when WiFi is on.
  const bool canread = _adc == 1 || WiFi.getMode() == WIFI_OFF;

  if (canread) {
# if HAS_TOUCH_GPIO

    if (readAsTouch && _isTouchPin) {
      return touchRead(_pin);
    }
# endif // if HAS_TOUCH_GPIO

# if ESP_IDF_VERSION_MAJOR >= 5

    int adc_raw{};

    if (ESP_OK == adc_oneshot_read(_adc_handle, _channel, &adc_raw)) {
      _lastADCvalue = adc_raw;
    }
# else // if ESP_IDF_VERSION_MAJOR >= 5

    _lastADCvalue = analogRead(_pin);

# endif // if ESP_IDF_VERSION_MAJOR >= 5
  }
  return _lastADCvalue;
}

float Hardware_ADC_t::applyFactoryCalibration(float rawValue) {
  return _adc_cali_handle.applyFactoryCalibration(rawValue);
}

/*
bool Hardware_ADC_t::hasADC_factory_calibration() {
# if ESP_IDF_VERSION_MAJOR < 5
  return esp_adc_cal_check_efuse(_adc_calibration_type) == ESP_OK;
# else // if ESP_IDF_VERSION_MAJOR < 5
  return false;
# endif // if ESP_IDF_VERSION_MAJOR < 5
}
*/

#endif // ifdef ESP32
