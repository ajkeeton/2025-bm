#include "stepper.h"

void stepper_t::dprintf(uint8_t level, const char *format, ...) {
  if (level > debug_level) {
    return;
  }

  char buf[1024];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  Serial.write(buf);
}

void stepper_t::choose_next() {
  switch (state) {
      case STEP_INIT:
        state = STEP_WAIT;
        set_onoff(STEPPER_OFF);
        
        #if 0
        dprintf(LOG_DEBUG, "%d: Init complete, doing bloom. Positions: %d -> %d,\n", 
            idx, position, pos_end);
        set_target(pos_end, settings_on_open);
        accel.set_pause_ms(500);
        #endif
        break;
      case STEP_SWEEP:
        dprintf(LOG_DEBUG, "%d: Doing next sweep\n", idx);
        choose_next_sweep();
        break;
      case STEP_CLOSE:
        // Special case, need to get away from the limit before changing direction
        dprintf(LOG_DEBUG, "%d: Close complete @ %d. Doing wiggle. fwd: %d\n", idx, position, forward);
        // TODO: random wiggle or just wait
        choose_next_wiggle();
        break;
      case STEP_WIGGLE:
        choose_next_wiggle();
        break;
      case STEP_BLOOM_WIGGLE:
        choose_next_bloom_wiggle();
        break;
      case STEP_HALF_BLOOM:
        choose_next_half_bloom_wiggle();
        break;
      case STEP_BLOOM:
        #ifdef SWEEP_ONLY
        state = STEP_INIT;
        set_target(0, settings_on_open);
        dprintf(LOG_DEBUG, "%d: Bloom complete, sweeping\n", idx);
        #else
        state = STEP_BLOOM_WIGGLE;
        dprintf(LOG_DEBUG, "%d: Bloom complete, doing wiggle\n", idx);
        // TODO: random wait or wiggle
        accel.set_pause_ms(750);
        choose_next_bloom_wiggle();
        #endif
        break;
    default:
      break;
  };
}

void stepper_t::choose_next_bloom_wiggle() {
  choose_next_wiggle(
    pos_end * settings_on_wiggle.pos_end_bloom_min, pos_end * settings_on_wiggle.pos_end_bloom_max);
  state = STEP_BLOOM_WIGGLE;
}

void stepper_t::choose_next_wiggle() {
  choose_next_wiggle(0, settings_on_wiggle.max_pos);
}

void stepper_t::choose_next_wiggle(int32_t lower, int32_t upper) {
  state = STEP_WIGGLE;

  uint32_t nxt = random(lower, upper);

  //Serial.printf( "%d: wiggle next: %d, r: %d, current %d, nxt: %d\n", idx, r, position, nxt);
  set_target(nxt, settings_on_wiggle);
  // XXX This has to go after set_target!
  accel.set_pause_ms(random(150, 750));

  // float a = 0; // settings_on_wiggle.accel / (random(0, 100)-50);
  // accel.accel_0 = settings_on_wiggle.accel + a;
  Serial.printf( "%d: wiggle next: from %ld to %ld\n", idx, position, nxt);
  //accel.delay_min = settings_on_wiggle.min_delay + random(100, 300);
}

void stepper_t::choose_next_half_bloom_wiggle() {
  state = STEP_HALF_BLOOM;

  uint32_t nxt = random(pos_end * .3, pos_end * .6);

  //Serial.printf( "%d: wiggle next: %d, r: %d, current %d, nxt: %d\n", idx, r, position, nxt);
  set_target(nxt, settings_on_wiggle);
  // XXX This has to go after set_target!
  accel.set_pause_ms(random(150, 750));

  //float a = 0;//  settings_on_wiggle.accel / (random(0, 100)-50);
  //accel.accel_0 = settings_on_wiggle.accel + a;
  Serial.printf( "%d: wiggle next: from %ld to %ld\n", idx, position, nxt);
  //accel.delay_min = settings_on_wiggle.min_delay + random(100, 300);
}

void stepper_t::choose_next_sweep() {
  accel.set_pause_ms(1000);
  
  if (position == pos_end) {
    dprintf(LOG_DEBUG, "%d: At end. Reversing\n", idx);
    set_target(0);
  } else {
    set_target(pos_end);
    dprintf(LOG_DEBUG, "%d: At start. Cooling off\n", idx);
    set_onoff(STEPPER_OFF);
  }

  accel.accel_0 = 0.00002;
  accel.delay_min = DELAY_MIN;
}

void stepper_t::randomize_delay() {
  accel.set_pause_ms(random(100, 1000));
  accel.delay_current = random(accel.delay_min, accel.delay_max);
}

void stepper_t::set_forward(bool f) {
  if (forward != f)
    accel.set_pause_min(); // Enforce short pause before reversing

  forward = f;
  if (forward) {
    digitalWrite(pin_dir, val_forward);
  } else {
    digitalWrite(pin_dir, val_backward);
  }
}

void stepper_t::set_target(int32_t tgt) {
  set_target(tgt, settings_on_wiggle);  // XXX use a default setting?
}

void stepper_t::set_target(int32_t tgt, const step_settings_t &ss) {
  pos_tgt = tgt;

  uint32_t steps_to_mid_point = abs(tgt - position) * .5;
  uint32_t maxd = ss.max_delay,
           mind = ss.min_delay;

  //accel.set_target(steps_to_mid_point, maxd, mind, ss.accel);
  accel.set_windowed_target(steps_to_mid_point, maxd, mind, 0.35);

  if (pos_tgt > position) {
    set_forward(true);
  } else if (pos_tgt < position) {
    set_forward(false);
  }

  dprintf(LOG_DEBUG, "%d: set_target: state: %d, current position: %ld, new target: %ld\n", 
      idx, state, position, pos_tgt);
  accel.set_pause_ms(ss.pause_ms);
}

void stepper_t::run() {
  if(disable)
    return;

  uint32_t now = millis();
  
  log();

  if(!accel.is_ready())
    return;

  set_onoff(STEPPER_ON);

  if(state == STEP_INIT || state == STEP_CLOSE) {
    //if(flex.dist_to_max() <= 1 && flex.debounced > 3) {
    // XXX The check for forward probably doesn't work when the sensor is backwards
    if(hall.is_triggered() && !forward && position != pos_tgt) {
      dprintf(LOG_DEBUG, "%d: Hall triggered. Pos: %d\n", 
          idx, position);
      position = 0;
      pos_tgt = position; // Stop. We'll hold using the delay via choose_next
      //delay(500);
    } 
  }

  #if 0
  if(state == STEP_BLOOM || state == STEP_INIT) {
    //if(flex.dist_to_max() <= 1 && flex.debounced > 3) {
    if(flex.at_max()) {
      dprintf(LOG_DEBUG, "%d: At high flex limit (%d)\n", idx, flex.dist_to_max());
      position = pos_end;
      pos_tgt = position; // Stop. We'll hold using the delay via choose_next
    }
  }
  #endif

  if (position != pos_tgt) {
    if (forward)
      position++;
    else
      position--;

    digitalWrite(pin_step, step_pin_val);
    step_pin_val = !step_pin_val;
    accel.next_s();
  }

  //if(!(position % 100) || position == pos_tgt)
  //  Serial.printf("%d: Position: %ld, Target: %ld (pulse delay: %u, pos_end: %ld)\n",
  //    idx, position, pos_tgt == -INT_MAX ? 0 : pos_tgt, accel.delay_current, pos_end);

  if (position == pos_tgt)
    choose_next();
}

void stepper_t::trigger_bloom() {
  if(state == STEP_BLOOM || state == STEP_BLOOM_WIGGLE)
    return;
  dprintf(LOG_DEBUG, "%d: Triggering bloom from state %d\n", idx, state);
  state = STEP_BLOOM;
  set_target(pos_end, settings_on_open);
}

void stepper_t::trigger_half_bloom() {
  if(state == STEP_HALF_BLOOM)
    return;
  dprintf(LOG_DEBUG, "%d: Triggering half bloom from state %d\n", idx, state);
  state = STEP_WIGGLE;
  set_target(pos_end/2, settings_on_wiggle);
}

void stepper_t::log() {
  if (debug_level < LOG_DEBUG)
    return;
  uint32_t now = millis();

  if(now - last_log < 500)
    return;

  uint32_t us = micros();
  last_log = now;

  const char* state_str = nullptr;
  switch (state) {
    case STEP_INIT: state_str = "init"; break;
    case STEP_BLOOM: state_str = "bloom"; break;
    case STEP_HALF_BLOOM: state_str = "half_bloom"; break;
    case STEP_BLOOM_WIGGLE: state_str = "bloom_wiggle"; break;
    case STEP_WIGGLE: state_str = "wiggle"; break;
    case STEP_CLOSE: state_str = "close"; break;
    case STEP_SWEEP: state_str = "sweep"; break;
    case STEP_WAIT: state_str = "wait"; break;
    default: state_str = "unknown"; break;
  }

  #if 0
  dprintf(LOG_DEBUG, "%d: Position: %ld, Target: %ld, State: %s, Fwd/Back: %d, "
      "Accel Delay: %ld, A0: %f, is ready: %d !(%lu && %lu)\n",
    idx, position, pos_tgt == -INT_MAX ? -99999 : pos_tgt,
    state_str, forward, 
    accel.delay_current, accel.accel_0, accel.is_ready(),
    us - accel.t_last_update < accel.t_pause_for,
    us - accel.t_last_update < accel.delay_current);
  #endif
    dprintf(LOG_DEBUG, "%d: Position: %ld, Target: %ld, State: %s, Fwd/Back: %d, "
      "Accel Delay: %ld, Window: %ld, Steps to target: %f, is ready: %d !(%lu && %lu)\n",
    idx, position, pos_tgt == -INT_MAX ? -99999 : pos_tgt,
    state_str, forward, 
    accel.delay_current,
    accel.window,
    accel.windowed_steps_to_target(), accel.is_ready(),
    us - accel.t_last_update < accel.t_pause_for,
    us - accel.t_last_update < accel.delay_current);
}