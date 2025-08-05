#include "common/common.h"
#include "common/minmax.h"
#include "common/wifi.h"
#include "stepper.h"
#include "leds.h"
#include "tof.h"

mux_t mux;
wifi_t wifi;
leds_t leds;
tof_t tof;
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
  ss.min_delay = 80;
  steppers[0].settings_on_close = ss;

  ss.pause_ms = 500;
  ss.min_delay = 80;
  steppers[1].settings_on_close = ss;

  ss.pause_ms = 1000;
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

  ss.accel = 0.000001;
  ss.pause_ms = 50;
  ss.min_delay = 400; 
  ss.max_delay = 20000;
  ss.min_pos = DEFAULT_MAX_STEPS * .20;
  ss.max_pos = DEFAULT_MAX_STEPS * .55; 

  steppers[0].settings_on_wiggle = ss;
  steppers[1].settings_on_wiggle = ss;
  steppers[2].settings_on_wiggle = ss;

  if(analogRead(INPUT_SWITCH_0) < 100) {
    Serial.printf("Pod 1\n");

    steppers[0].flex.init(SENS_FLEX_3, 330, 570);
    steppers[1].flex.init(SENS_FLEX_2, 330, 570);
    steppers[2].set_backwards();
    steppers[2].flex.init(SENS_FLEX_1, 340, 570);
  }
  else if (analogRead(INPUT_SWITCH_1) < 100) {
    // For limb C only!
    Serial.printf("Pod 3\n");
    steppers[1].set_backwards();
    steppers[2].set_backwards();
    steppers[0].flex.init(SENS_FLEX_1, 330, 570);
    steppers[1].flex.init(SENS_FLEX_2, 330, 570);
    steppers[2].flex.init(SENS_FLEX_3, 330, 570);
  }
  else {
    Serial.printf("Pod 2\n");
    steppers[0].flex.init(SENS_FLEX_1, 330, 570);
    steppers[1].flex.init(SENS_FLEX_2, 330, 570);
    steppers[2].flex.init(SENS_FLEX_3, 330, 570);
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
void wait_serial() {
  uint32_t now = millis();

  // Wait up to 2 seconds for serial
  while(!Serial && millis() - now < 2000) {
    delay(100);
  }  
}

void setup1() {
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

  mux.init();
  //mux.num_inputs = 6;
  init_steppers();
  init_mode();
  leds.init();

  minmax.init_avg(mux.read_raw(SENS_PROX_1));
}

void setup() {
  wait_serial();

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);

  tof.init();

  //wifi.init("venus");
  Serial.print("Chip ID: "); Serial.println(rp2040.getChipID());
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
  // minmax.log_info();

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
  
  // Move cores?
  leds.background_update();
  leds.step();
}

void loop() {
  mux.next();
  tof.next();

  /* 
  wifi.next();
  
  recv_msg_t msg;
  while(wifi.recv_pop(msg)) {
      switch(msg.type) {
          case PROTO_PULSE:
              Serial.printf("The gardener told us to pulse: %u %u %u %u\n", 
                  msg.pulse.color, msg.pulse.fade, msg.pulse.spread, msg.pulse.delay);
              break;
          case PROTO_STATE_UPDATE:
              Serial.printf("State update: %u %u\n", 
                      msg.state.pattern_idx, msg.state.score);
              break;
          case PROTO_PIR_TRIGGERED:
              Serial.println("A PIR sensor was triggered");
              break;
          case PROTO_SLEEPY_TIME:
              Serial.println("Received sleepy time message");
              break;
          default:
              Serial.printf("Ignoring message type: %u\n", msg.type);
      }
  }
  */
}
