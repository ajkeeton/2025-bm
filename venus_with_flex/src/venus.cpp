#include "common/common.h"
#include "common/minmax.h"
#include "common/hall.h"

// #include "common/wifi.h"
#include "stepper.h"
#include "leds.h"

bool setup0_done = false;
bool use_tof = false;

mux_t mux;
// wifi_t wifi;
leds_t leds;
hall_t hall;
debounce_t debounce;

min_max_range_t minmax(100, 1000); // Default min/max for the sensors

#define NUM_STEPPERS 3

stepper_t steppers[NUM_STEPPERS];

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
  ss.min_delay = 90;
  steppers[0].settings_on_close = ss;

  ss.pause_ms = 250;
  ss.min_delay = 90;
  steppers[1].settings_on_close = ss;

  ss.pause_ms = 500;
  ss.min_delay = 90;
  ss.accel = 0.00005;
  steppers[2].settings_on_close = ss;

  ss.pause_ms = 750;
  ss.accel = 0.00002;
  ss.min_delay = 120;
  steppers[0].settings_on_open = ss;

  ss.pause_ms = 400;
  ss.min_delay = 120;
  ss.accel = 0.00002;
  steppers[1].settings_on_open = ss;

  ss.pause_ms = 50;
  ss.min_delay = 120;
  ss.accel = 0.00002;
  steppers[2].settings_on_open = ss;

  ss.accel = 0.00001;
  ss.pause_ms = 50;
  ss.min_delay = 2000;
  ss.max_delay = 20000;
  ss.min_pos = DEFAULT_MAX_STEPS * .05;
  ss.max_pos = DEFAULT_MAX_STEPS * .5; 

  steppers[0].settings_on_wiggle = ss;
  steppers[1].settings_on_wiggle = ss;
  steppers[2].settings_on_wiggle = ss;

  if(analogRead(INPUT_SWITCH_0) < 100) {
    Serial.printf("Pod 1\n");
    use_tof = true;

    //steppers[0].set_backwards();
  }
  else if (analogRead(INPUT_SWITCH_1) < 100) {
    // For limb C only!
    Serial.printf("Pod 3\n");
    steppers[1].set_backwards();

    // Avoid a power surge from all starting at once
    delay(1000);
  }
  else {
    Serial.printf("Pod 2\n");
    steppers[1].set_backwards();

    // Avoid a power surge from all starting at once
    delay(500);
  }
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
  #endif

  for(int i=0; i<NUM_STEPPERS; i++) {
    steppers[i].choose_next(mode); // necessary to initialize targets, speeds, etc
    // steppers[i].state_next = mode; //  DEFAULT_MODE_NEXT; // STEP_SWEEP; // STEP_WIGGLE_START;
  }
}

void setup1() {
  while(!setup0_done) { yield(); }

  Serial.println("Setup1...");

  //mux.num_inputs = 6;
  Serial.println("Running...");
}

void setup() {
  pinMode(STEP_PULSE_1, OUTPUT);
  pinMode(STEP_PULSE_2, OUTPUT);
  pinMode(STEP_PULSE_3, OUTPUT);

  pinMode(STEP_DIR_1, OUTPUT);
  pinMode(STEP_DIR_2, OUTPUT);
  pinMode(STEP_DIR_3, OUTPUT);

  pinMode(STEP_EN_1, OUTPUT);
  pinMode(STEP_EN_2, OUTPUT);
  pinMode(STEP_EN_3, OUTPUT);

#ifdef USING_SINGLE_BOARD
  pinMode(STEP_PULSE_4, OUTPUT);
  pinMode(STEP_DIR_4, OUTPUT);
  pinMode(STEP_EN_4, OUTPUT);
#endif

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  wait_serial();

  mux.init();
  init_steppers();
 
  int sens = mux.read_raw(SENS_PROX_1);
  if(analogRead(INPUT_SWITCH_0) < 100) {
      use_tof = true;
    sens = constrain(map(sens, SENS_TOF_MIN, SENS_TOF_MAX, 30, 4), 4, 30);
    minmax.min_thold = 4;
    minmax.min_std_to_trigger = 4;
  }
  minmax.init_avg(sens);
  leds.init();
  init_mode();
  
  //wifi.init("venus");
  Serial.print("Chip ID: "); Serial.println(rp2040.getChipID());

  setup0_done = true;
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

  // NOTE: this will skew the measurement for the next iteration
  // Need extra work to keep accurate, but that's a pretty low priority
  if(now - last_output < 2000)
    return;

  Serial.printf("Benchmark: %f ms\n", avg);
  last_output = now;
}

void log_inputs() {
  static uint32_t last = 0;
  uint32_t now = millis();

  if(now - last < 250)
    return;
  last = now;

  mux.log_info();

  if(use_tof) {
    uint16_t tof = constrain(map(mux.read_raw(SENS_TOF_PIN), SENS_TOF_MIN, SENS_TOF_MAX, 30, 4), 4, 30);
    // To inches
    Serial.printf("ToF: %d, %.3f in\n", tof, (float)tof*0.393701);
  }
}

void loop1() {
  blink();
  // benchmark();

  log_inputs();

  #ifdef SENS_LOG_ONLY
  return;
  #endif

  // TODO: handle sensor override button 
  uint32_t sens = mux.read_raw(SENS_PROX_1);

  if(use_tof)
    sens = constrain(map(sens, SENS_TOF_MIN, SENS_TOF_MAX, 30, 4), 4, 30);
  
  minmax.update(sens);
  //Serial.printf("Sensor: %lu\n", sens);
  minmax.log_info(); 

  static uint32_t last_trigger = 0;
  uint32_t now = millis();
  bool triggered = false;
  if(use_tof) {
    triggered = debounce.update(sens < 11);
  }
  else {
    triggered = minmax.triggered_at(sens);
  }

  if(triggered) {
      if(now > last_trigger + 1500) {
        last_trigger = now;
        Serial.printf("Triggered: %ld (%.2f std: %.2f)\n", sens, minmax.avg, minmax.std_dev);
      }

      for(int i=0; i<NUM_STEPPERS; i++) {
        steppers[i].trigger_close();
      } 
  }

  for(int i=0; i<NUM_STEPPERS; i++)
    steppers[i].run();
}

void loop() {
  //Serial.println("mux.next...");
  mux.next();
  //Serial.println("mux.next done...");

  // LEDs should be on core 0!
  leds.background_update();
  leds.step();
}
