// Switching to hall sensors
#ifdef 0

#pragma once
#include <stdint.h>
#include "common/mux.h"

#define FLEX_DEF_MIN 300
#define FLEX_DEF_MAX 800
#define FLEX_N_DEBOUNCE 10

extern mux_t mux;

class flex_min_max_t {
public:
    void init(int p, int mn, int mx) {
        pin = p;
        min = mn;
        max = mx;
    }

    int dist_to_max() { 
        int cur = mux.read_raw(pin);
        uint32_t t_now = millis();
        // XXX Value falls as we open! XXX

        if (t_now - t_last_log >= 500) {
            Serial.printf("Flex pin %d cur - max: %d - %d = %d\n", pin, max, cur, cur-max);
            t_last_log  = t_now;
        }

        if(backwards)
            return max - cur;

        return cur - max;
    }

    bool at_max() {
        int d = dist_to_max();
        if(d < 1) {
            if(count_debounce++ < FLEX_N_DEBOUNCE)
                return false;
            return true;
        } 
        
        count_debounce = 0;
        return false;
    }

    int min = FLEX_DEF_MIN;
    int max = FLEX_DEF_MAX;
    uint32_t t_last_log = 0;
    int pin = 0;

    int count_debounce = 0;
    bool backwards = false;
};

#endif