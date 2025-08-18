#pragma once
#include "stepper.h"
#include "common/minmax.h"

enum BLOOM_STATE_T {
    BLOOM_INIT,
    BLOOM,
    BLOOM_CLOSE
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
    bool do_bloom(stepper_t &s);
    void do_bloom();
    void end_bloom();

    void do_wiggle(stepper_t &s);
    void do_wiggle();
    void do_close(stepper_t &s);
    void do_close();

    bool in_init();
    bool is_blooming();

    void check_activity();

    BLOOM_STATE_T state = BLOOM_INIT;
    stepper_t *inner = nullptr,
              *middle = nullptr,
              *outer = nullptr;

    uint32_t t_last_bloom = 0,
             t_last_activity = 0,
             t_last_close = 0;

    min_max_range_t minmax; 
};