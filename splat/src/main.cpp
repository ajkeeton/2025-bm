
#include "splat.h"
#include "common.h"

splat_t splat;
benchmark_t bloop0, bloop1;

int blink_delay = 500;
// To make sure things don't start running until after core 0 has finished setup()
bool ready_core_0 = false;

int button_state() {
  static uint16_t debounce = 0;
  EVERY_N_MILLISECONDS(5) {
      uint16_t but = analogRead(IN_BUTTON);
      if(but < 100) {
          switch(debounce) {
              case 3:
                  Serial.println("Button pressed");
                  debounce++;
              case 4:
                  break;
              break;
              default:
                  debounce++;
          }
      }
      else
          debounce = 0;
  }

  return debounce;
}

void loop1() {
  // NOTE: intentionally keeping blink the wifi core. 
  // That way we have info coming from cores, and if one locks up, we can tell which
  bloop0.start();
  blink();
  splat.next_core_0();
  bloop0.end();

  EVERY_N_MILLISECONDS(500) {
    Serial.printf("bloop0: %lu ms, bloop1: %lu ms\n", bloop0.elapsed(), bloop1.elapsed());
  }
}

void loop() {
  bloop1.start();
  blink(); 

  switch(button_state()) {
    case 3:
      splat.button_pressed();
      break;
    case 4:
      splat.button_hold();
    default: // debouncing
      break;
  }
  
  #ifdef FIND_SENS
  return find_sens();
  #endif

  #ifdef LED_TEST
  return led_test();
  #endif

  splat.next_core_1();
  bloop1.end();
}

void wait_serial() {
  uint32_t now = millis();
  // Wait up to 2 seconds for serial to be ready, otherwise just move on...
  while(!Serial && (millis() - now < 2000)) { 
    blink();
    delay(50);
  }
}

void setup1() {
  pinMode(IN_BUTTON, INPUT_PULLUP);
  pinMode(IN_DIP, INPUT_PULLUP);

  while(!ready_core_0) {}

  // For some reason this needs to be called for both cores?!
  // Otherwise, even with the check for ready_core_0, we just sail right on 
  // without serial ready
  wait_serial(); 

  Serial.printf("... SPLAT!\n");
}

void setup() {
  Serial.begin(9600);

  wait_serial();
  
  splat.init();

  ready_core_0 = true;
}
