#pragma once

#include "common.h"
#include "common/mux.h"
#include "accel.h"
#include "common/hall.h"
#include "common/bench.h"

#define STEP_LOG_DELAY 1000
#define DEFAULT_MAX_STEPS 7000

#define DELAY_MIN 80 // in MICRO seconds
#define DELAY_MAX 20000 // in MICRO seconds

extern mux_t mux;

enum STEP_STATE {
  STEP_INIT,
  STEP_WIGGLE_START,
  STEP_WIGGLE_END,
  STEP_SWEEP,
  STEP_CLOSE, // Close fingers to test range, confirm direction, etc
  STEP_OPEN,  // Opposite above, sets '0' based on limit switch
  STEP_TRIGGERED_INIT, // Open, prepare to grab
  STEP_GRAB,
  STEP_GRAB_WIGGLE,
  STEP_RELAX, // 90%'ish full open stretch
  STEP_DETANGLE, // reverse a little to help with detangling
};

enum STEP_PATTERN {
  PATTERN_INIT = 0,
  PATTERN_SUBDUED = 1,
  PATTERN_HIGH_ENERGY = 2,
  PATTERN_SLEEP = 3
};

#define DEFAULT_MODE STEP_INIT
#define DEFAULT_MODE_NEXT STEP_SWEEP

#define STEPPER_OFF false
#define STEPPER_ON true

#define MAX_DETANGLE_TRIES 4

enum TRIGGER_STAT {
    TRIGGER_OFF,
    TRIGGER_ON,
    TRIGGER_WAIT,
};

struct step_settings_t {
  uint32_t pause_ms = 50,
           min_delay = DELAY_MIN,
           max_delay = DELAY_MAX,
           min_pos = 0,
           max_pos = 0;
  float accel = 0.00005;
};

class stepper_t {
public:
  uint16_t idx = 0; // Which stepper this is, used for logging

  int32_t position = 1,
          pos_tgt = DEFAULT_MAX_STEPS, // Our target position
          pos_end = DEFAULT_MAX_STEPS;

  bool forward = true,
       was_on = false;
  int step_pin_val = 0;
  int how_wiggly = 0; // How much to wiggle when we grab
  
  STEP_STATE state = DEFAULT_MODE,
             state_next = DEFAULT_MODE_NEXT;

  int pin_step = 0,
      pin_dir = 0,
      pin_enable = 0;

  int val_forward = HIGH,
      val_backward = LOW;

  uint32_t last_log = 0;

  step_settings_t settings_on_close,
                  settings_on_open,
                  settings_on_wiggle;

  hall_t hall;
  accel_t accel;
  uint8_t debug_level = DEFAULT_LOG;

  int pattern = 0; // set by the gardener

  bench_t bench;

  void init(int i, int en, int step, int dir, int lsl, 
            int dmin = DELAY_MIN, int dmax = DELAY_MAX) {
    idx = i;
    pin_enable = en;
    pin_step = step;
    pin_dir = dir;

    pinMode(pin_enable, OUTPUT);
    pinMode(pin_step, OUTPUT);
    pinMode(pin_dir, OUTPUT);

    digitalWrite(pin_enable, HIGH); // Disable by default
    digitalWrite(pin_step, LOW);
    digitalWrite(pin_dir, val_forward);

    set_forward(true);

    was_on = true;
    set_onoff(STEPPER_OFF);
    accel.init(dmin, dmax);
    hall.init(lsl);
    do_init();
  }

  void do_init() {
    state = STEP_INIT;
    set_target(-DEFAULT_MAX_STEPS*2, settings_on_open);
  }

  void set_backwards() {
    val_forward = LOW;
    val_backward = HIGH;
  }

  void set_onoff(bool onoff) {
    if(onoff) {
      if(!was_on) {
        was_on = true;
        digitalWrite(pin_enable, LOW);
        accel.set_pause_min();
      }
    }
    else {
      if(was_on) {
        was_on = false;
        digitalWrite(pin_enable, HIGH);
      }
    }
  }

  void choose_next();
  void choose_next(STEP_STATE next);
  void choose_next_sweep();
  void choose_next_wiggle();
  void choose_next_wiggle(int32_t lower, int32_t upper);
  void randomize_delay();
  void set_forward(bool f);
  void set_target(int32_t tgt);
  void set_target(int32_t tgt, const step_settings_t &ss);
  void trigger_close();
  void run();
  void dprintf(uint8_t level, const char *format, ...);
  void log();
};
