#include <Arduino.h>
#include <M5Unified.h>
#include <SensirionI2cScd4x.h>
#include <Wire.h>

#include "config.h"

#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0

namespace {

SensirionI2cScd4x g_scd41;
char g_errorMessage[96] = {};

struct SensorValues {
    float temperatureC = NAN;
    float humidityPercent = NAN;
    uint16_t co2Ppm = 0;
    bool valid = false;
};

SensorValues g_values;
bool g_alertSoundEnabled = false;
bool g_sensorInitialized = false;
uint32_t g_nextAutoMeasureMs = 0;

bool isTemperatureAlert(const SensorValues& v) {
    return v.valid && v.temperatureC >= TEMP_ALERT_C;
}

bool isHumidityAlert(const SensorValues& v) {
    return v.valid && v.humidityPercent >= HUMIDITY_ALERT_PERCENT;
}

bool isCo2Alert(const SensorValues& v) {
    return v.valid && v.co2Ppm >= CO2_ALERT_PPM;
}

bool hasAnyAlert(const SensorValues& v) {
    return isTemperatureAlert(v) || isHumidityAlert(v) || isCo2Alert(v);
}

void errorToDisplayString(int16_t error, char* buffer, size_t size) {
    if (size == 0) {
        return;
    }
    if (error == NO_ERROR) {
        snprintf(buffer, size, "OK");
        return;
    }
    errorToString(error, buffer, size);
    buffer[size - 1] = '\0';
}

void drawFooter() {
    auto& lcd = M5.Display;
    lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
    lcd.setFont(&fonts::Font2);
    lcd.setTextSize(1);
    lcd.drawString("A: 手動測定    B: アラート音 ON/OFF", 10, 218);
}

void drawMessage(const char* line1, const char* line2 = nullptr, uint16_t color = TFT_WHITE) {
    auto& lcd = M5.Display;
    lcd.startWrite();
    lcd.fillScreen(TFT_BLACK);
    lcd.setFont(&fonts::lgfxJapanMinchoP_20);
    lcd.setTextSize(1);
    lcd.setTextColor(color, TFT_BLACK);
    lcd.drawString(line1, 14, 70);
    if (line2 != nullptr) {
        lcd.drawString(line2, 14, 105);
    }
    drawFooter();
    lcd.endWrite();
}

void drawRow(const char* label,
             const char* valueText,
             const char* unit,
             const char* status,
             bool statusIsAlert,
             int y) {
    auto& lcd = M5.Display;
    lcd.setFont(&fonts::lgfxJapanMinchoP_20);
    lcd.setTextSize(1);

    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.drawString(label, 10, y);
    lcd.drawString(valueText, 92, y);
    lcd.drawString(unit, 168, y);

    lcd.setTextColor(statusIsAlert ? TFT_RED : TFT_WHITE, TFT_BLACK);
    lcd.drawString(status, 232, y);
}

void drawValues() {
    auto& lcd = M5.Display;
    lcd.startWrite();
    lcd.fillScreen(TFT_BLACK);

    char tempText[16] = "--.-";
    char humidityText[16] = "--.-";
    char co2Text[16] = "----";

    if (g_values.valid) {
        snprintf(tempText, sizeof(tempText), "%4.1f", g_values.temperatureC);
        snprintf(humidityText, sizeof(humidityText), "%4.1f", g_values.humidityPercent);
        snprintf(co2Text, sizeof(co2Text), "%4u", g_values.co2Ppm);
    }

    const bool tempAlert = isTemperatureAlert(g_values);
    const bool humidityAlert = isHumidityAlert(g_values);
    const bool co2Alert = isCo2Alert(g_values);

    drawRow("温度", tempText, "℃", tempAlert ? "高温" : "快適", tempAlert, 28);
    drawRow("湿度", humidityText, "%", humidityAlert ? "不快" : "快適", humidityAlert, 72);
    drawRow("CO2値", co2Text, "ppm", co2Alert ? "不快" : "良好", co2Alert, 116);

    lcd.setFont(&fonts::lgfxJapanMinchoP_20);
    lcd.setTextSize(1);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.drawString(g_alertSoundEnabled ? "アラート音 ON" : "アラート音 OFF", 10, 164);

    drawFooter();
    lcd.endWrite();
}

void playAlertSoundIfNeeded() {
    if (!g_alertSoundEnabled || !hasAnyAlert(g_values)) {
        return;
    }

    // Short non-voice alert pattern. Tone() is supported by M5Unified's speaker API.
    M5.Speaker.setVolume(SPEAKER_VOLUME);
    for (int i = 0; i < 3; ++i) {
        M5.Speaker.tone(1800, 130);
        delay(170);
        M5.Speaker.tone(2400, 130);
        delay(220);
    }
    M5.Speaker.stop();
}

bool initializeSensor() {
    const int sda = M5.getPin(m5::pin_name_t::port_a_sda);
    const int scl = M5.getPin(m5::pin_name_t::port_a_scl);

    Serial.printf("PORT.A SDA=%d SCL=%d\n", sda, scl);
    Wire.begin(sda, scl, 400000U);

    g_scd41.begin(Wire, SCD41_I2C_ADDR_62);
    delay(30);

    // Bring the device to a known idle state. wakeUp()/stopPeriodicMeasurement()
    // may report a harmless error depending on the current state, so only reinit()
    // and serial number read are treated as fatal below.
    int16_t err = g_scd41.wakeUp();
    if (err != NO_ERROR) {
        errorToDisplayString(err, g_errorMessage, sizeof(g_errorMessage));
        Serial.printf("wakeUp(): %s\n", g_errorMessage);
    }

    err = g_scd41.stopPeriodicMeasurement();
    if (err != NO_ERROR) {
        errorToDisplayString(err, g_errorMessage, sizeof(g_errorMessage));
        Serial.printf("stopPeriodicMeasurement(): %s\n", g_errorMessage);
    }
    delay(500);

    err = g_scd41.reinit();
    if (err != NO_ERROR) {
        errorToDisplayString(err, g_errorMessage, sizeof(g_errorMessage));
        Serial.printf("reinit(): %s\n", g_errorMessage);
        return false;
    }
    delay(30);

    uint64_t serialNumber = 0;
    err = g_scd41.getSerialNumber(serialNumber);
    if (err != NO_ERROR) {
        errorToDisplayString(err, g_errorMessage, sizeof(g_errorMessage));
        Serial.printf("getSerialNumber(): %s\n", g_errorMessage);
        return false;
    }

    Serial.printf("SCD41 serial: 0x%08lX%08lX\n",
                  static_cast<unsigned long>(serialNumber >> 32),
                  static_cast<unsigned long>(serialNumber & 0xFFFFFFFFULL));
    return true;
}

bool readSensorSingleShot(SensorValues& out) {
    uint16_t co2 = 0;
    float temperature = NAN;
    float humidity = NAN;

    int16_t err = g_scd41.wakeUp();
    if (err != NO_ERROR) {
        errorToDisplayString(err, g_errorMessage, sizeof(g_errorMessage));
        Serial.printf("wakeUp(): %s\n", g_errorMessage);
        return false;
    }

    if (DISCARD_FIRST_SINGLE_SHOT_AFTER_WAKE) {
        err = g_scd41.measureSingleShot();
        if (err != NO_ERROR) {
            errorToDisplayString(err, g_errorMessage, sizeof(g_errorMessage));
            Serial.printf("measureSingleShot(): %s\n", g_errorMessage);
            return false;
        }
    }

    err = g_scd41.measureAndReadSingleShot(co2, temperature, humidity);
    if (err != NO_ERROR) {
        errorToDisplayString(err, g_errorMessage, sizeof(g_errorMessage));
        Serial.printf("measureAndReadSingleShot(): %s\n", g_errorMessage);
        return false;
    }

    if (co2 == 0 || isnan(temperature) || isnan(humidity)) {
        snprintf(g_errorMessage, sizeof(g_errorMessage), "invalid sensor data");
        Serial.println(g_errorMessage);
        return false;
    }

    out.co2Ppm = co2;
    out.temperatureC = temperature;
    out.humidityPercent = humidity;
    out.valid = true;

    Serial.printf("T=%.2f C, RH=%.2f %%, CO2=%u ppm\n", temperature, humidity, co2);
    return true;
}

void scheduleNextMeasurement(uint32_t delayMs) {
    g_nextAutoMeasureMs = millis() + delayMs;
}

void performMeasurement(const char* reasonText) {
    if (!g_sensorInitialized) {
        drawMessage("SCD41 再初期化中", nullptr, TFT_WHITE);
        g_sensorInitialized = initializeSensor();
        if (!g_sensorInitialized) {
            drawMessage("SCD41 初期化失敗", g_errorMessage, TFT_RED);
            scheduleNextMeasurement(SENSOR_ERROR_RETRY_MS);
            return;
        }
    }

    drawMessage(reasonText, "SCD41 測定中...", TFT_WHITE);

    SensorValues newValues;
    if (readSensorSingleShot(newValues)) {
        g_values = newValues;
        drawValues();
        playAlertSoundIfNeeded();
        scheduleNextMeasurement(SENSOR_UPDATE_INTERVAL_MS);
        return;
    }

    char line2[96];
    snprintf(line2, sizeof(line2), "%s", g_errorMessage);
    drawMessage("センサエラー", line2, TFT_RED);
    scheduleNextMeasurement(SENSOR_ERROR_RETRY_MS);
}

void handleButtons() {
    if (M5.BtnA.wasPressed()) {
        performMeasurement("手動測定");
    }

    if (M5.BtnB.wasPressed()) {
        g_alertSoundEnabled = !g_alertSoundEnabled;
        if (g_values.valid) {
            drawValues();
        } else {
            drawMessage(g_alertSoundEnabled ? "アラート音 ON" : "アラート音 OFF",
                        "初回測定待ち", TFT_WHITE);
        }
        playAlertSoundIfNeeded();
    }
}

}  // namespace

void setup() {
    Serial.begin(115200);

    M5.begin();
    M5.Speaker.setVolume(SPEAKER_VOLUME);

    auto& lcd = M5.Display;
    lcd.setRotation(1);
    lcd.setBrightness(160);
    lcd.fillScreen(TFT_BLACK);

    drawMessage("初期化中", "CO2L / SCD41", TFT_WHITE);

    g_sensorInitialized = initializeSensor();
    if (!g_sensorInitialized) {
        drawMessage("SCD41 初期化失敗", g_errorMessage, TFT_RED);
        scheduleNextMeasurement(SENSOR_ERROR_RETRY_MS);
        return;
    }

    performMeasurement("初回測定");
}

void loop() {
    M5.update();
    handleButtons();

    const uint32_t now = millis();
    if (static_cast<int32_t>(now - g_nextAutoMeasureMs) >= 0) {
        performMeasurement("定期測定");
    }

    delay(20);
}
