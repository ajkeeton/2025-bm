#include "common.h"
#include "common/mux.h"
#include "bloom.h"

void bloom_t::next() {
    handle_triggers();
    handle_timeouts();

    inner->run();
    middle->run();
    outer->run();

    if(log) {
        Serial.printf("Bloom: State: %d, t last activity: %lums)\n", 
            state,
            t_last_activity);
    }   
}

void bloom_t::handle_triggers() {    
    bool pir = mux.read_raw(SENS_REMOTE_1) > 120;
    bool sonar = mux.read_raw(SENS_REMOTE_2) > 120;

    if(!pir && !sonar)
        return;

    
    t_last_activity = millis();
    // Serial.printf("Bloom: handle triggers: PIR: %d, Sonar: %d\n", pir, sonar);

    // Our current state dictates behavior from triggrers
    switch(state) {
        case BLOOM_INIT:
            // Ignore triggers during initialization
            lprintf(log, "Triggered during init. Ignored\n");
            break;
        case BLOOM_WAIT:
            // Top does nothing on pir. Blooms on sonar
            // Lower petals should close on pir, open on sonar
            #ifdef BLOOM_TOP
            if(sonar) {
                Serial.println("Sonar triggered bloom from BLOOM_WAIT");
                do_bloom();
            }
            #else
            if(pir) {
                //Serial.printf("PIR triggered startle from BLOOM_WAIT\n");
                do_startle();
            }
            else {
                //Serial.printf("Sonar triggered bloom from BLOOM_WAIT\n");
                do_bloom();
            }
            #endif
            break;
        case BLOOM_FULL:
            // Already bloomed, do nothing
            break;
        default:
            Serial.printf("Top triggered, but ignoring state: %d\n", state);
    }

    /*
    Serial.printf("Bloom: After triggers: State: %d, Inner: %d/%d (%d), Middle: %d/%d (%d), Outer: %d/%d (%d)\n", 
        state,
        inner->position, inner->pos_tgt, inner->state,
        middle->position, middle->pos_tgt, middle->state,
        outer->disable ? -1 : outer->position, 
        outer->disable ? -1 : outer->pos_tgt, 
        outer->disable ? -1 : outer->state);
    */
}

void bloom_t::handle_timeouts() {
    uint32_t now = millis();

    // Our current state dictates behavior from triggrers
    switch(state) {
        case BLOOM_INIT:
            // Ignore triggers during initialization
            break;
        case BLOOM_WAIT:
            #ifndef BLOOM_TOP
            if(now - t_last_close > TIMEOUT_PETALS_OPEN) {
                if(!t_last_bloom) {
                    Serial.printf("Lower petals open timeout\n");
                }

                do_bloom();
                state = BLOOM_WAIT;
            }
            #endif
            break;
        case BLOOM_FULL:
            if(t_last_activity && now - t_last_activity > TIMEOUT_BLOOM) {
                Serial.printf("Bloom timed out. Closing: %lums\n", now - t_last_activity);
                end_bloom();
                state = BLOOM_WAIT;
            } 
            else {
               // Serial.printf("Bloom active. Not timing out (%lums < %lums)\n", 
               //     now - t_last_activity, TIMEOUT_BLOOM);
            }
            break;
        default:
            break;
    }
}

void bloom_t::init_stepper(stepper_t &s) {
    s.set_init();
    
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
            (!outer->disable && outer->state == STEP_INIT));
}

bool bloom_t::is_blooming() {
    return (inner->state == STEP_BLOOM ||
            middle->state == STEP_BLOOM ||
            (!outer->disable && outer->state == STEP_BLOOM));
}

bool bloom_t::is_bloomed() {
    return (inner->state == STEP_BLOOM_WIGGLE ||
            middle->state == STEP_BLOOM_WIGGLE ||
            (!outer->disable && outer->state == STEP_BLOOM_WIGGLE));
}

void bloom_t::init() {
    // Home each petal, in sequence
    init_stepper(*outer);
    init_stepper(*middle);
    init_stepper(*inner);

    #ifdef BLOOM_A
    while(inner->state == STEP_INIT) { 
        inner->run();
        blink();
    }
    
    while(middle->state == STEP_INIT) {
        middle->run();
        blink();
    }

    while(!outer->disable && outer->state == STEP_INIT) {
        outer->run();
        blink();
    }
    
    #else
    while(in_init()) {
        outer->run();
        middle->run();
        inner->run();
        blink();
    }
    #endif

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
    if(s.disable) return;
    if(s.state != STEP_BLOOM_WIGGLE && s.state != STEP_BLOOM)
        return;

    t_last_close = millis();
    s.state = STEP_CLOSE;
    s.set_target(0, s.settings_on_close);
}

bool bloom_t::do_bloom(stepper_t &s) {
    if(s.disable) return true;

    s.trigger_bloom();

    return true;
}

void bloom_t::do_bloom() {
    if(state == BLOOM_FULL)
        return;

    if(is_blooming() || is_bloomed())
        return;

    // No instant reblooms
    if(millis() - t_last_close < TIMEOUT_REBLOOM) {
        if(log) 
            Serial.printf( "Delaying rebloom, just closed %lu ms ago\n", 
                millis() - t_last_close);
        return;
    }

    //if(t_last_bloom)
    // ...

    Serial.printf("Doing bloom from state %d. Inner state: %d\n", 
            state, inner->state);
    t_last_bloom = millis();
    t_last_close = 0;
    state = BLOOM_FULL;

    if(!do_bloom(*outer)) return;
    if(!do_bloom(*middle)) return;
    if(!do_bloom(*inner)) return;
}

void bloom_t::end_bloom() {
    if(state != BLOOM_FULL && state != BLOOM_INIT)
        return;
    
    t_last_bloom = 0;
    t_last_close = millis();

    Serial.printf("Ending bloom\n");

    // NOTE: when a bloom ends, go ahead and retract the lower petals
    // They'll expand again after a timeout

    do_close(*inner);
    do_close(*middle);
    do_close(*outer);
    t_last_close = millis();
    state = BLOOM_WAIT;
}

void bloom_t::do_startle() {
    // Lower petals should close, but only if we were waiting
    if(state != BLOOM_WAIT)
        return;

    uint32_t now = millis();
    if(now - t_last_close < 1000)   
        return;
    t_last_close = now;

    //lprintf(true, "Startled. Closing (%lu > 120)\n", 
    //    mux.read_raw(SENS_REMOTE_1));
    //if(log) 
        Serial.printf("Startled. Closing (%lu > 120)\n", 
            mux.read_raw(SENS_REMOTE_1)); 

    do_close(*inner);
    do_close(*middle);

    t_last_close = millis();
    t_last_bloom = 0;
    state = BLOOM_WAIT;
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

#if 0
void bloom_t::check_activity() {
    int remote = mux.read_raw(SENS_REMOTE_1);
    if(remote > 200) {
        t_last_activity = millis();
    }
}
#endif