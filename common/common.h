#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <Wire.h>

#define BLINK_DELAY 750 // ms
#define LOG_TIMEOUT 500 // in ms

void blink();
void wait_serial();
void lprintf(bool enabled, const char* fmt, ...);

struct log_throttle_t {
  uint32_t last = 0;
  uint32_t log_timeout = LOG_TIMEOUT;
  bool should_log() {
    uint32_t now = millis();
    if(now - last < log_timeout)
      return false;

    last = now;
    return true;
  }
};

class debounce_t {
  public:
    bool update(bool state) {
      uint32_t now = millis();
      if(state != last_state) {
        last_update = now;
        last_state = state;
      }
      else if(now - last_update >= interval) {
        stable_state = last_state;
      }
      return stable_state;
    }

  private:
    uint32_t interval = 50;
    bool last_state = false,
         stable_state = false;
    uint32_t last_update = 0;
};

inline void i2c_scan(TwoWire& wire = Wire) {
  Serial.println("Scanning I2C bus...");
  for (uint8_t address = 1; address < 127; ++address) {
    wire.beginTransmission(address);
    uint8_t error = wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println();
    }
  }
  Serial.println("I2C scan complete.");
}