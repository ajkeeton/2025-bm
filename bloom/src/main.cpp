#include "common/common.h"
#include "stepper.h"
//#include "leds.h"
#include "bloom.h"

// IDs:
// 16B6410F28393B76 [current dev]

bloom_t bloom;
mux_t mux;
// leds_t leds;

#define NUM_STEPPERS 3
stepper_t steppers[NUM_STEPPERS];

void init_steppers() {
  steppers[0].init(0, STEP_EN_1, STEP_PULSE_1, STEP_DIR_1, 
                   SENS_LIMIT_1, DELAY_MIN, DELAY_MAX);
  steppers[1].init(1, STEP_EN_2, STEP_PULSE_2, STEP_DIR_2, 
                   SENS_LIMIT_2, DELAY_MIN, DELAY_MAX);

  step_settings_t ss;

  #ifdef BLOOM_TOP
  steppers[2].init(2, STEP_EN_3, STEP_PULSE_3, STEP_DIR_3, 
                   SENS_LIMIT_3, DELAY_MIN, DELAY_MAX);

  ss.pause_ms = 10;
  ss.accel = 0.00001;
  ss.min_delay = 300;
  steppers[0].settings_on_close_fast = ss;

  ss.pause_ms = 500;
  ss.min_delay = 400;
  steppers[1].settings_on_close_fast = ss;

  ss.pause_ms = 1000;
  ss.min_delay = 500;
  ss.accel = 0.00005;
  steppers[2].settings_on_close_fast = ss;


  ss.pause_ms = 10;
  ss.accel = 0.00001;
  ss.min_delay = 300;
  steppers[0].settings_on_close_slow = ss;

  ss.pause_ms = 500;
  ss.min_delay = 400;
  steppers[1].settings_on_close_slow = ss;

  ss.pause_ms = 1000;
  ss.min_delay = 500;
  ss.accel = 0.00005;
  steppers[2].settings_on_close_fast = ss;


  ss.pause_ms = 1000;
  ss.accel = 0.00001/2;
  ss.min_delay = 500;
  steppers[0].settings_on_open = ss;

  ss.pause_ms = 500;
  ss.min_delay = 350;
  ss.accel = 0.00001/2;
  steppers[1].settings_on_open = ss;

  ss.pause_ms = 50;
  ss.min_delay = 250;
  ss.accel = 0.00001/2;
  steppers[2].settings_on_open = ss;

  ss.accel = 0.000001;
  ss.pause_ms = 50;
  ss.min_delay = 400; 
  ss.max_delay = 20000;
  ss.min_pos = 0;
  ss.max_pos = DEFAULT_MAX_STEPS * .2;
  steppers[0].settings_on_wiggle = ss;
  steppers[1].settings_on_wiggle = ss;
  steppers[2].settings_on_wiggle = ss;

  steppers[0].pos_end = DEFAULT_MAX_STEPS * 1.2;
  steppers[1].pos_end = DEFAULT_MAX_STEPS;
  steppers[2].pos_end = DEFAULT_MAX_STEPS;

  steppers[0].set_backwards();
  steppers[2].set_backwards();
#else
  steppers[0].set_backwards();
  steppers[1].set_backwards();

  // Bloom B - The long petals
  steppers[2].disable = true; // No 3rd stepper on Bloom B

  ss.pause_ms = 10;
  ss.accel = 0.00001;
  ss.min_delay = 100;
  steppers[0].settings_on_close_fast = ss;

  ss.pause_ms = 50;
  ss.min_delay = 200;
  steppers[1].settings_on_close_fast = ss;

  ss.pause_ms = 100;
  ss.accel = 0.000005;
  ss.min_delay = 400;
  steppers[0].settings_on_close_slow = ss;

  ss.pause_ms = 500;
  ss.min_delay = 500;
  steppers[1].settings_on_close_slow = ss;

  ss.pause_ms = 750;
  ss.accel = 0.000005;
  ss.min_delay = 550;
  steppers[0].settings_on_open = ss;

  ss.pause_ms = 500;
  ss.min_delay = 400;
  ss.accel = 0.000005;
  steppers[1].settings_on_open = ss;

  ss.accel = 0.0000015;
  ss.pause_ms = 50;
  ss.min_delay = 400; 
  ss.max_delay = 20000;
  ss.min_pos = 0;
  ss.max_pos = DEFAULT_MAX_STEPS * .35;
  steppers[0].settings_on_wiggle = ss;
  steppers[1].settings_on_wiggle = ss;

  steppers[0].pos_end = DEFAULT_MAX_STEPS;
  steppers[1].pos_end = DEFAULT_MAX_STEPS;
#endif

  bloom.add_steppers(steppers[0], steppers[1], steppers[2]);
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

void setup1() {
  wait_serial();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println("Starting...");

  mux.init(8);
  init_steppers();
  init_mode();

  bloom.init();
}

void setup() {
  Serial.begin(9600);

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);

  pinMode(SIGNAL_IN_BLOOM1, OUTPUT);
  pinMode(SIGNAL_IN_BLOOM2, OUTPUT);
  digitalWrite(SIGNAL_IN_BLOOM1, LOW);
  digitalWrite(SIGNAL_IN_BLOOM2, LOW);

  // leds.init();

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

  if(now - last < 250)
    return;

  last = now;

  mux.log_info();
}

void loop1() {
  blink();
  // benchmark();
      
  #ifdef SENS_LOG_ONLY
  return;
  #endif

  bloom.log = false;

  static uint32_t last_log = 0;
  uint32_t now = millis();

  if (now - last_log >= 500) {
    bloom.log = true;
    last_log = now;
  }
  else {
    bloom.log = false;
  } 

  bloom.next();

  if(bloom.is_bloomed()) {
    digitalWrite(SIGNAL_IN_BLOOM1, HIGH);
    digitalWrite(SIGNAL_IN_BLOOM2, HIGH);
  } else {
    digitalWrite(SIGNAL_IN_BLOOM1, LOW);
    digitalWrite(SIGNAL_IN_BLOOM2, LOW);
  }

  log_inputs();
}

void loop() {
  mux.next();
  //leds.background_update();
  //leds.step();
}
