#pragma once

#include "common.h"
#include "common/mux.h"
#include "accel.h"
#include "common/hall.h"

#define STEP_LOG_DELAY 1000

#ifdef BLOOM_TOP
#define DEFAULT_MAX_STEPS 4500
#else
#define DEFAULT_MAX_STEPS 11000
#endif

#define DELAY_MIN 80 // in MICRO seconds
#define DELAY_MAX 20000 // in MICRO seconds

extern mux_t mux;

enum STEP_STATE {
  STEP_INIT,
  STEP_SWEEP,
  STEP_CLOSE,
  STEP_WIGGLE,
  STEP_BLOOM,
  STEP_BLOOM_WIGGLE,
  STEP_HALF_BLOOM, // For the lower petals
  STEP_WAIT
};

enum STEP_PATTERN {
  PATTERN_INIT = 0,
  PATTERN_SUBDUED = 1,
  PATTERN_HIGH_ENERGY = 2,
  PATTERN_SLEEP = 3
};

#define DEFAULT_MODE STEP_INIT

#define STEPPER_OFF false
#define STEPPER_ON true

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
  float
           pos_end_bloom_min = 0.75, // % of pos_end
           pos_end_bloom_max = .99;
  float accel = 0.000005;
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
  
  STEP_STATE state = DEFAULT_MODE;
             // state_next = DEFAULT_MODE_NEXT;

  int pin_step = 0,
      pin_dir = 0,
      pin_enable = 0;

  int val_forward = HIGH,
      val_backward = LOW;

  uint32_t last_log = 0;

  step_settings_t settings_on_close_fast,
                  settings_on_close_slow,
                  settings_on_open,
                  settings_on_wiggle;

  accel_t accel;
  hall_t hall;

  uint8_t debug_level = DEF_LOG;

  int pattern = 0; // set by the gardener
  int detangling = 0; // How many times we've tried to detangle

  bool disable = false;

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

    was_on = true;
    set_onoff(STEPPER_OFF);
    accel.init(dmin, dmax);
    hall.init(lsl);
    disable = false;
  }

  void set_init() {
    state = STEP_INIT;
    set_forward(false);
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
  void choose_next_sweep();
  void choose_next_rand_walk();
  void choose_next_wiggle();
  void choose_next_wiggle(int32_t lower, int32_t upper);
  void choose_next_bloom_wiggle();
  void choose_next_half_bloom_wiggle();
  void randomize_delay();
  void set_forward(bool f);
  void set_target(int32_t tgt);
  void set_target(int32_t tgt, const step_settings_t &ss);
  void trigger_bloom();
  void trigger_half_bloom();
  void run();
  void dprintf(uint8_t level, const char *format, ...);
  void log();
};
