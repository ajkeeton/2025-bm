#pragma once

#include <string>
#include "strips.h"
#include "common.h"

// Milliseconds to wait before sending sensor updates
// #define T_REMOTE_SENSOR_UPDATE_DELAY 100

struct benchmark_t {
    uint32_t tstart = 0;
    uint32_t avg = 0;

    void start() {
        tstart = millis();
    }
    uint32_t elapsed() {
        return millis() - tstart;
    }
    void end() {
        avg *= 4;
        avg += elapsed();
        avg /= 5;
    }
};

class splat_t {
public:
    // Throttles how often we send sensor updates
    uint32_t t_last_sensor_update_sent = 0;
    // The LED strips
    strip_t strips[MAX_STRIPS];
    int nstrips = 0;

    benchmark_t bench_led_push, 
                bench_led_calcs,
                bench_sensors;

    void log_info();

    void next_core_0();
    void next_core_1();

    uint16_t num_strips() {
        return nstrips;
    }

    void button_pressed() {
    }

    void button_hold() {
    }

    void init();
    void strips_next();
};