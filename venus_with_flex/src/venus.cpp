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
  ss.min_delay = 20;
  steppers[0].settings_on_close = ss;

  ss.pause_ms = 250;
  ss.min_delay = 80;
  steppers[1].settings_on_close = ss;

  ss.pause_ms = 500;
  ss.min_delay = 80;
  ss.accel = 0.00005;
  steppers[2].settings_on_close = ss;

  ss.pause_ms = 1000;
  ss.accel = 0.00003;
  ss.min_delay = 80;
  steppers[0].settings_on_open = ss;

  ss.pause_ms = 500;
  ss.min_delay = 80;
  ss.accel = 0.00003;
  steppers[1].settings_on_open = ss;

  ss.pause_ms = 50;
  ss.min_delay = 80;
  ss.accel = 0.00003;
  steppers[2].settings_on_open = ss;

  ss.accel = 0.00001;
  ss.pause_ms = 50;
  ss.min_delay = 200;
  ss.max_delay = 20000;
  ss.min_pos = DEFAULT_MAX_STEPS * .15;
  ss.max_pos = DEFAULT_MAX_STEPS * .6; 

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

  Serial.begin(9600);
  wait_serial();

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

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println("Starting...");

  //mux.num_inputs = 6;
  init_steppers();
  init_mode();

  minmax.init_avg(mux.read_raw(SENS_PROX_1));
}

void setup() {
  mux.init();
  leds.init();

  wait_serial();

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);

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

  if(use_tof) {
    uint16_t tof = constrain(map(mux.read_raw(SENS_TOF_PIN), SENS_TOF_MIN, SENS_TOF_MAX, 30, 4), 4, 30);
    // To inches
    Serial.printf("ToF: %d, %.3f in\n", mux.read_raw(SENS_TOF_PIN), (float)tof*0.393701);
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

  minmax.update(sens);
  // Serial.printf("Sensor: %d, Min: %d, Max: %d\n", sens, minmax.avg_min, minmax.avg_max);
  //minmax.log_info();

  static uint32_t last = 0;
  uint32_t now = millis();

  if(minmax.triggered_at(sens)) {
      if(now > last + 500) {
        last = now;
        Serial.printf("Triggered: %ld (%.2f std: %.2f)\n", sens, minmax.pseudo_avg, minmax.std_dev);
      }

      for(int i=0; i<NUM_STEPPERS; i++) {
        steppers[i].trigger_close();
      } 
  }

  for(int i=0; i<NUM_STEPPERS; i++)
    steppers[i].run();
}

void loop() {
  mux.next();
  // Move cores?
  leds.background_update();
  leds.step();
}
