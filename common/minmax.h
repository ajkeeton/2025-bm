#pragma once
#include "common.h"

// It would be nice if this were over time, not total samples, but keeping it 
// simple for now
#define N_MOVING_AVG 50
#define MIN_STD_DEV_FOR_TRIGGER 70 // Used to determine threshold
#define T_CALC_STD 1000
#define MIN_THOLD 70 // Needed for noisy floating pins (even though there's a pulldown)
#define DEF_MIN_MAX_LOG_TIMEOUT 250

#define INIT_TRIG_THOLD 280 
#define INIT_TRIG_MAX 700
#define TRIG_SPREAD_MIN 250
#define TRIG_DECAY_DELTA_MIN 150 // ms
#define TRIG_DECAY_DELTA_MAX 500 // ms

struct min_max_range_t {
    float 
        avg_min = INIT_TRIG_THOLD, 
        avg_max = INIT_TRIG_MAX,
        max_max = 0;

    float window[N_MOVING_AVG];
    uint32_t t_last_update_window = 0;

    int widx = 0;
    float
        avg = 0,
        std_dev = 0,
        std_dev_min = 0,
        std_dev_max = 0,
        gradient = 0;

    uint32_t last_decay_min = 0,
             last_decay_max = 0,
             min_thold = MIN_THOLD,
             min_std_to_trigger = MIN_STD_DEV_FOR_TRIGGER;
    log_throttle_t log;
    
    min_max_range_t() {
        init();
    }

    min_max_range_t(uint32_t min, uint32_t max) 
        : avg_min(min), avg_max(max), max_max(max) {
        init();
    }

    void init() {
        last_decay_min = last_decay_max = millis();
        log.log_timeout = 250;
        for(int i=0; i<N_MOVING_AVG; i++)
            window[i] = 0;
        t_last_update_window = millis();
    }
    
    void init_avg(uint16_t val) {
        for(int i=0; i<N_MOVING_AVG; i++)
            window[i] = val;
        update_window(val);
    }

    void update(uint16_t val);
    void update_window(uint16_t val);

    float get_thold() const;
    bool triggered_at(uint16_t val);
    // Slowly close the min/max range
    void decay(); 
    void log_info();
};
