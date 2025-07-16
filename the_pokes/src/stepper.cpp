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
          dprintf(LOG_DEBUG, "%d: Init complete\n", idx);
          position = 0;
          dprintf(LOG_DEBUG, "%d: Doing next sweep\n", idx);
          choose_next_sweep();
          break;
      case STEP_SWEEP:
          dprintf(LOG_DEBUG, "%d: Doing next sweep\n", idx);
          choose_next_sweep();
          break;
    #if 0
      case STEP_INIT:
        state = STEP_CLOSE;
        dprintf(LOG_DEBUG, "Init complete, doing close. Pos: %d, fwd: %d\n", 
            position, forward);
        set_target(0, settings_on_close);
        accel.set_pause_ms(500);
        dprintf(LOG_DEBUG, "%d: Close next target: %d, fwd: %d\n", 
          idx, pos_tgt, forward);
        break;
      case STEP_SWEEP:
        dprintf(LOG_DEBUG, "%d: Doing next sweep\n", idx);
        choose_next_sweep();
        break;
      case STEP_CLOSE:
        // Special case, need to get away from the limit before changing direction
        
        if(flex.dist_to_max() < 1) {
          position += 250;
          accel.set_pause_ms(500);
          dprintf(LOG_DEBUG, "%d: Closed but currently past limit. "
              "Backing off, pos: %d, tgt: %d, fwd: %d\n", 
              idx, position, pos_tgt, forward);
        }
        else 
        {
          dprintf(LOG_DEBUG, "%d: Closed @ %d. Doing init. fwd: %d\n", idx, position, forward);

          // TODO: random wiggle or just wait
          choose_next_wiggle();
          //set_onoff(STEPPER_OFF);

          dprintf(LOG_DEBUG, "%d: Init next target: %d, fwd: %d\n", idx, pos_tgt, forward);
        }

        break;
      case STEP_WIGGLE:
        choose_next_wiggle();
        break;
      case STEP_BLOOM_WIGGLE:
        choose_next_bloom_wiggle();
        break;
      case STEP_BLOOM:
        state = STEP_BLOOM_WIGGLE;
        dprintf(LOG_DEBUG, "%d: Bloom complete, doing wiggle\n", idx);
        // TODO: random wait or wiggle
        accel.set_pause_ms(1000);
        choose_next_bloom_wiggle();
        break;
    default:
      break;
    #endif
  };
}

void stepper_t::choose_next_bloom_wiggle() {
  choose_next_wiggle(pos_end*.92, pos_end*.99);
  state = STEP_BLOOM_WIGGLE;
}

void stepper_t::choose_next_wiggle() {
  choose_next_wiggle(0, settings_on_wiggle.max_pos);
}

void stepper_t::choose_next_wiggle(int32_t lower, int32_t upper) {
  state = STEP_WIGGLE;
  accel.set_pause_ms(random(500, 1500));

  //set_onoff(STEPPER_OFF);
  uint32_t r = random(lower, upper);
  int32_t nxt = 0;
  if (position != 0 && random() & 1)
    nxt = position - r;
  else
    nxt = position + r;

  nxt = min(nxt, upper);
  nxt = max(lower, nxt);

  set_target(nxt, settings_on_wiggle);

  if(nxt != position)
    dprintf(LOG_DEBUG, "%d: wiggle next: from %ld to %ld\n", idx, position, nxt);
}

void stepper_t::choose_next_sweep() {
  state = STEP_SWEEP;
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

  accel.set_target(steps_to_mid_point, maxd, mind, ss.accel);

  if (pos_tgt > position) {
    set_forward(true);
  } else if (pos_tgt < position) {
    set_forward(false);
  }

  accel.set_pause_ms(ss.pause_ms);

  //Serial.printf("%d: Position: %d, New target: %d End: %d fwd/back: %d\n",
  //  idx, position, pos_tgt == -INT_MAX ? 0 : pos_tgt, pos_end, forward);
}

void stepper_t::run() {
  uint32_t now = millis();
  
  if (debug_level >= LOG_DEBUG && now - last_log > 1000) {
    uint32_t us = micros();
    last_log = now;

    dprintf(LOG_DEBUG, "%d: Position: %ld, Target: %ld, State: %d, Fwd/Back: %d, "
        "Accel Delay: %ld, A0: %f, is ready: %d !(%lu && %lu)\n",
      idx, position, pos_tgt == -INT_MAX ? -99999 : pos_tgt,
      state, forward, 
      accel.delay_current, accel.accel_0, accel.is_ready(),
      us - accel.t_last_update < accel.t_pause_for,
      us - accel.t_last_update < accel.delay_current);
  }

  if (!accel.is_ready())
    return;

  set_onoff(STEPPER_ON);

  TRIGGER_STAT ls = limits.check_triggered();

  switch (ls) {
    case TRIGGER_ON:
      dprintf(LOG_DEBUG, "%d: At low limit\n", idx);
      position = 0;
      set_target(10);
      break;
    case TRIGGER_WAIT:
      // We may technically change modes and back into the LS again
      // while triggered
      // Force us to continue moving away from switch even though triggered
      if (!forward) {
        // Force us to choose a new direction/mode
        position = 0;
        set_target(10);
      }
      break;
    default:
      break;
  }

  if (position != pos_tgt) {
    if (forward)
      position++;
    else
      position--;

    digitalWrite(pin_step, step_pin_val);
    step_pin_val = !step_pin_val;
    accel.next_plat();
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
    
//  if(state != STEP_WIGGLE)
//    return;

  state = STEP_BLOOM;
  set_target(pos_end, settings_on_open);
}