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

#define MIN_LED 3
#define MAX_LED_ON_BLOOM 200
#define MAX_LED_ON_DIM 25

class stamen_t {
public:
    void init() {
        // For the LED
        pinMode(STEP_EN_4, OUTPUT);
        pinMode(STEP_PULSE_4, OUTPUT);
        pinMode(STEP_DIR_4, OUTPUT);
        digitalWrite(STEP_EN_4, LOW);
        digitalWrite(STEP_PULSE_4, LOW);
        digitalWrite(STEP_DIR_4, LOW);

        step.init(3, STEP_EN_3, STEP_PULSE_3, STEP_DIR_3, 
                    SENS_LIMIT_3, 90, 5000);
        step.debug_level = LOG_DEBUG;
        step.pos_end = 22000;
        
        step_settings_t ss;
        ss.min_pos = 0;
        ss.max_delay = 5000;
        ss.min_delay = 90;
        ss.pause_ms = 50;
        ss.accel = 0.00005;
        ss.max_pos = step.pos_end;
        step.settings_on_close_slow = ss;
        step.settings_on_close_fast = ss;

        ss.max_delay = 2000;
        step.settings_on_open = ss;

        ss.min_delay = 500;
        ss.max_pos = 1000;
        ss.pos_end_bloom_min = 0.7;
        ss.pos_end_bloom_max = 1.0;
        step.settings_on_wiggle = ss;

        // step.state = STEP_SWEEP;
        // step.set_target(step.pos_end);
        step.set_backwards();
        step.set_init();
    }

    void next() {
        #ifdef BLOOM_TOP
        return;
        #endif

        step.run();

        //Serial.printf("Stamen: State: %d, Pos: %d/%d (%d) - now: %lu, last: %lu\n", 
        //    step.state, step.position, step.pos_tgt, step.state, millis(), t_last);
        uint32_t now = millis();
        if(now - t_delay < 20) 
            return;
        t_delay = now;

        // Time since t_bloom determines brightness, up to 2 seconds
        if(t_bloom) {
            uint32_t dt = now - t_bloom;
            if(dt < 1000)
                return;
            dt -= 1000;
            if(dt > 6000) dt = 6000;
            uint8_t v = map(dt, 0, 6000, MIN_LED, MAX_LED_ON_BLOOM);
            // Only change when v is greater than the curent value
            r = v > r ? r = v : r;
            g = v > g ? g = v : g;

            // Blue moves faster
            if(dt > 2000) dt = 2000;
            v = map(dt, 0, 2000, MIN_LED, MAX_LED_ON_BLOOM);
            b = v > b ? b = v : b;
        } else if(t_close) {
            uint32_t dt = now - t_close;
            if(dt > 2000) dt = 2000;
            uint8_t v = map(dt, 0, 2000, MAX_LED_ON_BLOOM, MIN_LED);
            // Only change when v is less than the current value
            r = v < r ? r = v : r;
            g = v < g ? g = v : g;

            // Blue moves slower
            dt = now - t_close > 6000 ? 6000 : now - t_close;
            v = map(dt, 0, 6000, MAX_LED_ON_BLOOM, MIN_LED);
            if(dt > 6000) dt = 6000;
            b = v < b ? b = v : b;

            if(r <= MIN_LED && g <= MIN_LED && b <= MIN_LED) {
                Serial.println("Stamen: t_close complete");
                t_close = 0;
            }
        } else {
            // A slow pulsing when idle calculated from t_last_led
            uint32_t dt = now - t_last_led;

            if(dt > 20) {
                t_last_led = now;

                if(random(0,100) < 20) {
                    if(r > 1 && rdir < 0)
                        r += rdir;
                    //else if(r < MAX_LED_ON_DIM && rdir > 0)
                    //    r += rdir;
                }

                if(random(0,100) < 20) {
                    if(g > 1 && gdir < 0)
                        g += gdir;
                    //else if(g < MAX_LED_ON_DIM && gdir > 0)
                    //    g += gdir;
                }

                if(random(0,100) < 20) {
                    if(b > MIN_LED && bdir < 0)
                        b += bdir;
                    else if(b < MAX_LED_ON_DIM && bdir > 0)
                        b += bdir;
                }
            }

            if(now - t_last_dir > 500) {
                t_last_dir = now;
                
                if(r >= MAX_LED_ON_DIM)
                    rdir = -1;//random(1,3);
                if(r <= MIN_LED)
                    rdir = 1;

                if(g >= MAX_LED_ON_DIM)
                    gdir = -1;//random(1,3);
                if(g <= MIN_LED)
                    gdir = 1;

                if(b >= MAX_LED_ON_DIM)
                    bdir = -1;
                if(b <= MIN_LED)
                    bdir = 1; // random(1,2);
            }
        }

        analogWrite(STEP_EN_4, r);
        analogWrite(STEP_PULSE_4, g);
        analogWrite(STEP_DIR_4, b);

        //Serial.printf("Stamen LED: %d/%d, %d/%d, %d/%d\n", r, rdir, g, gdir, b, bdir);
    }

    void do_bloom() {
        #ifdef BLOOM_TOP
        return;
        #endif
        step.trigger_bloom();

        t_bloom = millis();
        t_close = 0;
    }

    void do_close() {
        #ifdef BLOOM_TOP
        return;
        #endif
        step.state = STEP_CLOSE;
        step.set_target(0, step.settings_on_close_fast);

        t_close = millis();
        t_bloom = 0;
    }

    stepper_t step;

    uint8_t r = 0, g = 0, b = 0;
    int8_t rdir = 1, gdir = 1, bdir = 1;
    uint32_t t_delay = 0,
        t_last_led = 0,
        t_last_dir = 0,
        t_bloom = 0,
        t_close = 0;
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

    stamen_t stamen;

    uint32_t t_last_bloom = 0,
             t_last_activity = 0,
             t_last_close = 0;

    min_max_range_t minmax; 

    bool log = false;
};
