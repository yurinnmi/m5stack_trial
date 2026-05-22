#pragma once

#include <Arduino.h>

// Sensor update interval required by the specification.
static constexpr uint32_t SENSOR_UPDATE_INTERVAL_MS = 2UL * 60UL * 1000UL;

// Retry interval after a sensor read error. Kept shorter than the normal interval
// so that wiring/power-up mistakes recover without waiting two minutes.
static constexpr uint32_t SENSOR_ERROR_RETRY_MS = 30UL * 1000UL;

// Alert thresholds required by the specification.
static constexpr float TEMP_ALERT_C = 28.0f;
static constexpr float HUMIDITY_ALERT_PERCENT = 70.0f;
static constexpr uint16_t CO2_ALERT_PPM = 1000;

// SCD41 single-shot note:
// Sensirion's official SCD41 single-shot example discards the first measurement
// after wake-up. This makes Button A / 2-minute updates slower, but avoids showing
// a stale first sample after the sensor wakes.
static constexpr bool DISCARD_FIRST_SINGLE_SHOT_AFTER_WAKE = true;

// Internal speaker alert. This project implements the requested "音案内" as
// a buzzer pattern. For real spoken guidance, add audio assets/TTS separately.
static constexpr uint8_t SPEAKER_VOLUME = 96;  // 0..255
