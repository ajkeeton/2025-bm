#include "common.h"
#include "common/mux.h"
#include "bloom.h"

void bloom_t::next() {
    uint32_t sens = mux.read_raw(SENS_TOF_1);
    uint32_t override = mux.read_raw(SENS_PIR_1);

    minmax.update(sens);

    // Serial.printf("Sensor: %d, Min: %d, Max: %d\n", sens, minmax.avg_min, minmax.avg_max);
    // minmax.log_info();

    static uint32_t last = 0;
    uint32_t now = millis();

    if(override > 512 || minmax.triggered_at(sens)) {
        if(now > last + 500) {
            last = now;
            Serial.printf("Triggered: sens: %ld, override: %ld, (%.2f std: %.2f)\n", 
                    sens, override, minmax.pseudo_avg, minmax.std_dev);
        }

        do_bloom();
    }
    else if(now - t_last_bloom > 15000 && t_last_bloom != 0) {
        Serial.printf("Bloom timeout. Ending after %lu ms\n", now - t_last_bloom);
        end_bloom();
    }

    inner->run();
    middle->run();
    outer->run();
}

void bloom_t::init_stepper(stepper_t &s) {
    s.do_init();
    
    //s.set_target(s.pos_end*2, s.settings_on_open);    
    /*
    while(s.state == STEP_INIT) {
        s.run();
        delay(1);

        // If enough time has lapsed, assume we're stuck,
        // reverse direction, and try again
        // XXX ^^^^^^^^^ handle in stepper code?
    }
    */
}

bool bloom_t::in_init() {
    return (inner->state == STEP_INIT ||
            middle->state == STEP_INIT ||
            outer->state == STEP_INIT);
}

bool bloom_t::is_blooming() {
    return (inner->state == STEP_BLOOM ||
            middle->state == STEP_BLOOM ||
            outer->state == STEP_BLOOM);
}

void bloom_t::init() {
    // Home each petal, in sequence
    init_stepper(*outer);
    init_stepper(*middle);
    init_stepper(*inner);

    while(in_init()) {
        outer->run();
        middle->run();
        inner->run();
        blink();
    }

    Serial.println("Close complete. Doing initial bloom");

    do_bloom();

    while(is_blooming()) {
        outer->run();
        middle->run();
        inner->run();
        blink();
    }

    Serial.println("Initial bloom complete");

    end_bloom();
}

void bloom_t::do_close(stepper_t &s) {
    s.state = STEP_CLOSE;
    s.set_target(0, s.settings_on_close);
    //while(s.state == STEP_CLOSE) {
    //    s.run();
    //    delay(1);

        // If enough time has lapsed, assume we're stuck,
        // reverse direction, and try again
    //}
}

bool bloom_t::do_bloom(stepper_t &s) {
    s.trigger_bloom();
    #if 0
    uint32_t t_start = millis();
    while(s.state == STEP_BLOOM) {
        s.run();
        delay(1);

        // XXX Revisit! Use flex sensors to detect if we're stuck, not a timeout
        if(millis() - t_start > 5000) {
            // XXX Handle stuck!!!
            Serial.printf("Bloom timeout on stepper %d\n", s.idx);
            return false;
        }
    }
    #endif

    return true;
}

void bloom_t::do_bloom() {
    t_last_bloom = millis();
    state = BLOOM;

    if(!do_bloom(*outer)) return;
    if(!do_bloom(*middle)) return;
    if(!do_bloom(*inner)) return;
}

void bloom_t::end_bloom() {
    do_close(*inner);
    do_close(*middle);
    do_close(*outer);
    t_last_bloom = 0;
    state = BLOOM_CLOSE;
}

#if 0
void bloom_t::do_wiggle(stepper_t *s) {
    inner->run();
    middle->run();
    outer->run();
}

void bloom_t::do_wiggle() {
    do_wiggle(inner);
    do_wiggle(middle);
    do_wiggle(outer);
}
#endif