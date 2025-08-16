#if 0

#pragma once
#include <stdint.h>
#include "common/mux.h"
#include "bench.h"

#define FLEX_DEF_MIN 300
#define FLEX_DEF_MAX 800
#define FLEX_N_AV 20
#define FLEX_N_DEBOUNCE 20

extern mux_t mux;

class flex_min_max_t {
public:
    void init(int p, int mn, int mx) {
        pin = p;
        min = mn;
        max = mx;
    }

    #if 0
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
    #endif

    int dist_to_min() { 
        int cur = mux.read_raw(pin);
        uint32_t t_now = millis();
        // XXX Value falls as we open! XXX

        int d;
        if(backwards) d = min-cur;
        else d = cur-min; 

        #if 0
        if (t_now - t_last_log >= 250) {
            Serial.printf("Flex pin %d: min: %d, cur: %d diff: %d\n", 
                pin, min, cur, d);
            t_last_log = t_now;
        }
        #endif

        return d;
    }

    bool at_min() {
        int d = dist_to_min();
 
        avg = avg * (FLEX_N_AV - 1) + d;
        avg /= FLEX_N_AV;

        if(avg < 1)
            return true;

        #if 0
        if(d < 1) {
            if(count_debounce++ < FLEX_N_DEBOUNCE)
                return false;
            return true;
        } 
        
        count_debounce = 0;
        #endif
        return false;
    }

    bool is_zero() {
        int cur = mux.read_raw(pin);
       
        if(cur < 100) { // Reading ~80 when power is off
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
    uint64_t avg = 0;
    bool backwards = false;
};

#endif