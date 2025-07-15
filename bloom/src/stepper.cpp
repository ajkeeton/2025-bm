#include "stepper.h"

void Stepper::dprintf(uint8_t level, const char *format, ...) {
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

void Stepper::choose_next() {
  switch (state) {
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
          state = STEP_WIGGLE;
          set_target(pos_end, settings_on_open);
          accel.set_pause_ms(500);
          
          // STEP_WIGGLE; 
          //choose_next_wiggle();
          //set_onoff(STEPPER_OFF);

          dprintf(LOG_DEBUG, "%d: Init next target: %d, fwd: %d\n", idx, pos_tgt, forward);
        }

        delay(500);
        break;
      case STEP_WIGGLE:
        choose_next_wiggle();
        break;
      case STEP_BLOOM:
        state = STEP_CLOSE;
        set_target(0);
        // TODO: random wait or wiggle
        accel.set_pause_ms(1000);
        break;
    default:
      break;
  };
}


void Stepper::choose_next_wiggle() {
  choose_next_wiggle(0, settings_on_wiggle.max_pos);
}

void Stepper::choose_next_wiggle(int32_t lower, int32_t upper) {
  accel.set_pause_ms(random(500, 2000));

  set_onoff(STEPPER_OFF);
  uint32_t r = random(lower, upper);
  int32_t nxt = 0;
  if (position != 0 && random() & 1)
    nxt = position - r;
  else
    nxt = position + r;

  nxt = min(nxt, upper);
  nxt = max(lower, nxt);

  set_target(nxt);

  if(nxt != position)
    dprintf(LOG_DEBUG, "%d: wiggle next: from %ld to %ld\n", idx, position, nxt);
}

void Stepper::choose_next_sweep() {
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

void Stepper::randomize_delay() {
  accel.set_pause_ms(random(100, 1000));
  accel.delay_current = random(pulse_delay_min, pulse_delay_max);
}

void Stepper::set_forward(bool f) {
  if (forward != f)
    accel.set_pause_min(); // Enforce short pause before reversing

  forward = f;
  if (forward) {
    digitalWrite(pin_dir, val_forward);
  } else {
    digitalWrite(pin_dir, val_backward);
  }
}

void Stepper::set_target(int32_t tgt) {
  set_target(tgt, settings_on_wiggle);  // XXX use a default setting?
}

void Stepper::set_target(int32_t tgt, const step_settings_t &ss) {
  pos_tgt = tgt;

// Args:
//  - Scale by number of steps to the target full speed or full stop
//  - Max delay
//  - Min delay
//  - A constant to scale the calc
#if 0
  if(state == STEP_WIGGLE_START || state == STEP_WIGGLE_END)
    accel.set_target(abs(tgt - position)*.5, DELAY_MAX, 750, 0.0000025);
  else
    accel.set_target(abs(tgt - position)*.5, DELAY_MAX/2, 100, 0.000005);
#endif

  uint32_t steps_to_mid_point = abs(tgt - position) * .5;
  uint32_t maxd = ss.max_delay,
           mind = ss.min_delay;

  accel.set_target(steps_to_mid_point, maxd, mind, ss.accel);

  if (pos_tgt > position) {
    set_forward(true);
  } else if (pos_tgt < position) {
    set_forward(false);
  }

  //Serial.printf("%d: Position: %d, New target: %d End: %d fwd/back: %d\n",
  //  idx, position, pos_tgt == -INT_MAX ? 0 : pos_tgt, pos_end, forward);
}

void Stepper::run() {
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

  if(state == STEP_BLOOM || state == STEP_INIT) {
    //if(flex.dist_to_max() <= 1 && flex.debounced > 3) {
    if(flex.at_max()) {
      dprintf(LOG_DEBUG, "%d: At high flex limit (%d)\n", idx, flex.dist_to_max());
      position = pos_end;
      pos_tgt = position; // Stop. We'll hold using the delay via choose_next
    }
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

void Stepper::trigger_bloom() {
  if(state != STEP_WIGGLE)
    return;

  state = STEP_BLOOM;
  set_target(pos_end, settings_on_open);
}