#include "common/common.h"
#include "common/mux.h"
#include "stepper.h"

mux_t mux;

#define NUM_STEPPERS 4

stepper_t steppers[NUM_STEPPERS];

void init_steppers() {
  steppers[0].init(0, STEP_EN_1, STEP_PULSE_1, STEP_DIR_1, 
                   SENS_LIM_1, DELAY_MIN, DELAY_MAX);
  steppers[1].init(1, STEP_EN_2, STEP_PULSE_2, STEP_DIR_2, 
                   SENS_LIM_2, DELAY_MIN, DELAY_MAX);
  steppers[2].init(2, STEP_EN_3, STEP_PULSE_3, STEP_DIR_3, 
                   SENS_LIM_3, DELAY_MIN, DELAY_MAX);
  steppers[3].init(3, STEP_EN_4, STEP_PULSE_4, STEP_DIR_4, 
                   SENS_LIM_4, DELAY_MIN, DELAY_MAX);

  step_settings_t ss;

  ss.pause_ms = 10;
  ss.accel = 0.00001;
  ss.min_delay = 120;
  steppers[0].settings_on_close = ss;

  ss.pause_ms = 500;
  ss.min_delay = 120;
  steppers[1].settings_on_close = ss;

  ss.pause_ms = 1000;
  ss.min_delay = 120;
  ss.accel = 0.00005;
  steppers[2].settings_on_close = ss;

  ss.pause_ms = 1000;
  ss.accel = 0.00001;
  ss.min_delay = 120;
  steppers[0].settings_on_open = ss;

  ss.pause_ms = 500;
  ss.min_delay = 120;
  ss.accel = 0.00001;
  steppers[1].settings_on_open = ss;

  ss.pause_ms = 50;
  ss.min_delay = 120;
  ss.accel = 0.00001;
  steppers[2].settings_on_open = ss;

  ss.accel = 0.000001;
  ss.pause_ms = 50;
  ss.min_delay = 750; 
  ss.max_delay = 20000;
  ss.min_pos = 0;
  ss.max_pos = DEFAULT_MAX_STEPS * .15; 
  steppers[0].settings_on_wiggle = ss;
  steppers[1].settings_on_wiggle = ss;
  steppers[2].settings_on_wiggle = ss;
}

void wait_serial() {
  uint32_t now = millis();

  // Wait up to 2 seconds for serial
  while(!Serial && millis() - now < 2000) {
    delay(100);
  }  
}

void setup1() {
  wait_serial();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println("Starting...");

  mux.init(8);
  init_steppers();
}

void setup() {
  Serial.begin(9600);

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);

  wait_serial();

  Serial.print("Chip ID: "); Serial.println(rp2040.getChipID());

  //wifi.init("bloom");
}

void benchmark() {
  static uint32_t iterations = 0;
  static float avg = 0;
  static uint32_t last = 0;
  static uint32_t last_output = millis();

  uint32_t now = millis();
  uint32_t diff = now - last;
  last = now;

  if(!iterations)
    avg = diff;
  else
    // Poor-mans moving average
    avg = (avg * 4 + diff)/5;

  // NOTE: This will skew the measurement for the next iteration
  // Need extra work to keep accurate, but that's a pretty low priority
  if(now - last_output < 2000)
    return;

  Serial.printf("Benchmark: %f ms\n", avg);
  last_output = now;
}

void log_inputs() {
  static uint32_t last = 0;
  uint32_t now = millis();

  if(now - last < 750)
    return;

  last = now;

  mux.log_info();
}

void run_steppers() {
  for (int i = 0; i < NUM_STEPPERS; i++) {
    steppers[i].run();
  }
}

void loop1() {
  blink();
  // benchmark();
  run_steppers();
}

void loop() {
  mux.next();
  log_inputs();

  //leds.background_update();
  //leds.step();
}
