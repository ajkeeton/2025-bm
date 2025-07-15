#include "common/common.h"
#include "common/minmax.h"
#include "common/wifi.h"
#include "stepper.h"
#include "leds.h"

// IDs:
// 16B6410F28393B76 [current dev]

mux_t mux;
leds_t leds;
min_max_range_t minmax(100, 1000); // Default min/max for the sensors

#define NUM_STEPPERS 3

Stepper steppers[NUM_STEPPERS];

void init_steppers() {
  steppers[0].init(0, STEP_EN_1, STEP_PULSE_1, STEP_DIR_1, 
                   SENS_FLEX_1, DELAY_MIN, DELAY_MAX);
  steppers[1].init(1, STEP_EN_2, STEP_PULSE_2, STEP_DIR_2, 
                   SENS_FLEX_2, DELAY_MIN, DELAY_MAX);
  steppers[2].init(2, STEP_EN_3, STEP_PULSE_3, STEP_DIR_3, 
                   SENS_FLEX_3, DELAY_MIN, DELAY_MAX);
 
  step_settings_t ss;

  ss.pause_ms = 10;
  ss.accel = 0.0001;
  ss.min_delay = 80;
  steppers[0].settings_on_close = ss;

  ss.pause_ms = 500;
  ss.min_delay = 120;
  steppers[1].settings_on_close = ss;

  ss.pause_ms = 1000;
  ss.min_delay = 140;
  ss.accel = 0.00005;
  steppers[2].settings_on_close = ss;

  ss.pause_ms = 1000;
  ss.accel = 0.00005;
  ss.min_delay = 80;
  steppers[0].settings_on_open = ss;

  ss.pause_ms = 500;
  ss.min_delay = 150;
  ss.accel = 0.00001;
  steppers[1].settings_on_open = ss;

  ss.pause_ms = 50;
  ss.min_delay = 300;
  ss.accel = 0.0001;
  steppers[2].settings_on_open = ss;

  #if 0
  ss.pause_ms = 10;
  ss.accel = 0.0001;
  ss.min_delay = 80;
  steppers[0].settings_on_close = ss;
  
  ss.pause_ms = 10;
  steppers[1].settings_on_close = ss;

  // Wait to close
  ss.accel = 0.000025;
  ss.pause_ms = 300;
  ss.min_delay = 100;
  steppers[2].settings_on_close = ss;

  // Open slower
  ss.pause_ms = 250;
  ss.min_delay = 150;
  ss.accel = 0.000015;
  steppers[0].settings_on_open = ss;
  steppers[1].settings_on_close = ss;

  // Open faster than the others
  ss.pause_ms = 10;
  ss.min_delay = 50;
  ss.accel = 0.00015;
  steppers[2].settings_on_open = ss;
  #endif

  ss.accel = 0.000002;
  ss.pause_ms = 50;
  ss.min_delay = 750; 
  ss.max_delay = 10000;
  ss.min_pos = 200;
  ss.max_pos = DEFAULT_MAX_STEPS * .1; 
  steppers[0].settings_on_wiggle = ss;
  steppers[1].settings_on_wiggle = ss;
  steppers[2].settings_on_wiggle = ss;

  steppers[0].flex.init(SENS_FLEX_1, 348, 200);
  steppers[1].flex.init(SENS_FLEX_2, 470, 580);
  steppers[1].flex.backwards = true;

// XXX BACKWARDS
  //steppers[2].flex.init(SENS_FLEX_3, );

  steppers[0].pos_end = DEFAULT_MAX_STEPS * 1.25;
  steppers[1].pos_end = DEFAULT_MAX_STEPS ;
  steppers[2].pos_end = DEFAULT_MAX_STEPS * .8;
}

void init_mode() {
  STEP_STATE mode = DEFAULT_MODE;

  #if 0
  int i1 = mux.read_switch(INPUT_SWITCH_0);
  int i2 = mux.read_switch(INPUT_SWITCH_1);
  int i3 = 0; //int i3 = mux.read_switch(INPUT_SWITCH_2);

  Serial.print("Setting mode to: ");

  switch(i1 + i2 << 1 + i3 << 2) {
    case 1:
      mode = STEP_CLOSE;
      //mode_next = STATE_OPEN;
      Serial.println("close");
      break;
    case 2:
      mode = STEP_OPEN;
      //mode_next = STATE_CLOSE;
      Serial.println("open");
      break;
    case 3:
      mode = STEP_SWEEP;
      Serial.println("sweep");
      break;
    case 4:
      mode = STEP_RANDOM_WALK;
      Serial.println("random walk");
    case 0:
    default:
      Serial.printf("default (%d)\n", mode);
      break;
  };

  for(int i=0; i<NUM_STEPPERS; i++) {
    steppers[i].state = mode;
    steppers[i].choose_next(); // necessary to initialize targets, speeds, etc
  }
  #endif
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
  init_mode();

  minmax.init_avg(mux.read_raw(SENS_TOF_1));
}

void setup() {
  Serial.begin(9600);

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);

  leds.init();

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

bool trigger_bloom() {
  for(int i=0; i<NUM_STEPPERS; i++)
    if(steppers[i].state != STEP_WIGGLE)
        return false;

  for(int i=0; i<NUM_STEPPERS; i++)
    steppers[i].trigger_bloom();

  return true;
}

void loop1() {
  blink();
  // benchmark();

  mux.next();

  log_inputs();

  // TODO: handle sensor override button 
  uint32_t sens = mux.read_raw(SENS_TOF_1);
  uint32_t override = mux.read_raw(0);

  minmax.update(sens);

  // Serial.printf("Sensor: %d, Min: %d, Max: %d\n", sens, minmax.avg_min, minmax.avg_max);
  // minmax.log_info();

  static uint32_t last = 0;
  uint32_t now = millis();

  if(override > 512 || minmax.triggered_at(sens)) {
      if(now > last + 500) {
        last = now;
        Serial.printf("Triggered: sens: %ld, override: %ld, (%.2f std: %.2f)\n", 
                sens, override, minmax.pseudo_avg, minmax.std_dev);
      }

      trigger_bloom();
  }

  //for(int i=0; i<2; i++)
  //  steppers[i].run();
  steppers[1].run();
}

void loop() {
  leds.background_update();
  leds.step();
}
