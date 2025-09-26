#pragma once
#include "stepper.h"
#include "common/minmax.h"

#define TIMEOUT_BLOOM 5000 // ms to wait before auto-closing bloom
#define TIMEOUT_REBLOOM 500 // ms to wait before we rebloom
#define TIMEOUT_PETALS_OPEN 5000 // Lower petals open. Should be a high number

enum BLOOM_STATE_T {
    BLOOM_INIT,
    BLOOM_WAIT, // Lower petals open, top closed
    BLOOM_HALF, // Lower petals half open
    BLOOM_FULL, // All open
   //  BLOOM_CLOSE, // All closed
};

enum SPEED_T {
    SPEED_SLOW,
    SPEED_FAST,
    SPEED_RAND
};

class bloom_t {
public:
    bloom_t() {
        minmax = min_max_range_t(100, 1000);
    }

    void init_stepper(stepper_t &s);
    void init();

    void add_steppers(stepper_t &s1, stepper_t &s2, stepper_t &s3) {
        inner = &s1;
        middle = &s2;
        outer = &s3;
    }

    void add_steppers(stepper_t &s1, stepper_t &s2) {
        inner = &s1;
        outer = &s2;
    }

    void next();

    void handle_triggers();
    void handle_timeouts();
    
    bool do_half_bloom(stepper_t &s);
    void do_half_bloom();
    bool do_bloom(stepper_t &s);
    void do_bloom();

    void end_bloom();

    void do_wiggle(stepper_t &s, SPEED_T speed);
    void do_wiggle();
    void do_close(stepper_t &s, SPEED_T speed);
    void do_close();
    // Only applies to lower
    void do_startle();

    bool in_init();
    bool is_blooming();
    bool is_bloomed();
    void check_activity();

    BLOOM_STATE_T state = BLOOM_INIT;
    stepper_t *inner = nullptr,
              *middle = nullptr,
              *outer = nullptr;

    uint32_t t_last_bloom = 0,
             t_last_activity = 0,
             t_last_close = 0;

    min_max_range_t minmax; 

    bool log = false;
};