#include "common.h"
#include <Arduino.h>

void blink() {
    static bool onoff = false;
    static uint32_t last = 0;

    uint32_t now = millis();
    if(now - last < BLINK_DELAY) 
        return;

    last = now;
    onoff = !onoff;
    digitalWrite(LED_BUILTIN, onoff);
}

void wait_serial() {
  uint32_t now = millis();

  // Wait up to 2 seconds for serial
  while(!Serial && millis() - now < 2000) {
    delay(100);
  }  
}