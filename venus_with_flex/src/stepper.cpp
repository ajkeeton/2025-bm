#include "stepper.h"

void stepper_t::dprintf(uint8_t level, const char *format, ...) {
  if (level > debug_level) {
    return;
  }

  char buf[2048];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  Serial.write(buf);
}

void stepper_t::choose_next() {
  choose_next(state_next);
}

void stepper_t::choose_next(STEP_STATE next) {
  state = next;
  int r;
  switch (state) {
    case STEP_INIT:
      //if (DEFAULT_MODE_NEXT == STEP_SWEEP)
      //  state_next = STEP_SWEEP;
      //else
        state_next = STEP_CLOSE;

      dprintf(LOG_DEBUG, "%d: Initializing\n", idx);
      set_target(-DEFAULT_MAX_STEPS*4);

      // Override max speed to avoid slamming into the limit
      accel.delay_min = max(accel.delay_min, 200);
      accel.accel_0 = 0.00015;
      break;
    case STEP_WIGGLE_START:
      state_next = STEP_WIGGLE_END;
      dprintf(LOG_DEBUG, "%d: Wiggle start\n", idx);
      choose_next_wiggle();
      break;
    case STEP_WIGGLE_END:
      //if(!(random() % 6))
      //  state_next = STEP_CLOSE;
      r = random();
      //dprintf(LOG_DEBUG, "%d: Wiggle end. Random: %d, %d\n", idx, r, r%8);  
      if (!(r % 8))
        state_next = STEP_OPEN;
      else {
        state_next = STEP_WIGGLE_START;
      }

      dprintf(LOG_DEBUG, "%d: Wiggle end. Next state: %d\n", idx, state_next);
      // set_onoff(STEPPER_OFF);
      choose_next_wiggle();
      break;
    case STEP_CLOSE:
      state_next = STEP_OPEN;
      set_target(pos_end, settings_on_close);
      accel.set_pause_ms(10);
      dprintf(LOG_DEBUG, "%d: Doing close\n", idx);
      break;
    case STEP_OPEN:
      set_target(-DEFAULT_MAX_STEPS*.01, settings_on_open);  // Force us to find the lower limit
      accel.set_pause_ms(250);
      dprintf(LOG_DEBUG, "%d: Doing open\n", idx);
      #ifdef OPEN_CLOSE_ONLY
      state_next = STEP_CLOSE;
      #else
      state_next = STEP_RELAX;
      #endif
      break;
    case STEP_RELAX:
      state_next = STEP_WIGGLE_START;
      // set_onoff(STEPPER_OFF);
      accel.set_pause_ms(500);
      dprintf(LOG_DEBUG, "%d: Doing relax\n", idx);
      break;
    case STEP_TRIGGERED_INIT:
      dprintf(LOG_DEBUG, "%d: Doing triggered init\n", idx);
      // Start by opening a little 
      //set_target(position * 0.9, settings_on_close);
      //state_next = STEP_GRAB;
      //break;
    case STEP_GRAB:
      set_target(pos_end, settings_on_close);
      accel.set_pause_ms(settings_on_close.pause_ms);
      dprintf(LOG_DEBUG, "%d: Doing grab\n", idx);
      state_next = STEP_GRAB_WIGGLE;
      break;
    case STEP_GRAB_WIGGLE:
      choose_next_wiggle(pos_end * .85, pos_end * .99);
      accel.set_pause_ms(random(10, 150));
      dprintf(LOG_DEBUG, "%d: Doing grab wiggle\n", idx);

      if(--how_wiggly > 0) {
        state_next = STEP_GRAB_WIGGLE;
      } else {
        state_next = STEP_OPEN;
      }
      break;
    case STEP_SWEEP:
      choose_next_sweep();
      dprintf(LOG_DEBUG, "%d: Doing sweep\n", idx);
      break;
    default:
      dprintf(LOG_ERROR, "%d: Unknown state %d\n", idx, state);
      state_next = STEP_INIT; // Reset to a known state
      break;
  };
}

void stepper_t::choose_next_wiggle() {
  choose_next_wiggle(settings_on_wiggle.min_pos, settings_on_wiggle.max_pos);
}

void stepper_t::choose_next_wiggle(int32_t lower, int32_t upper) {
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
  accel.set_pause_ms(random(150, 750));
  float a = settings_on_wiggle.accel / (random(50, 100));
  accel.accel_0 = settings_on_wiggle.accel + a;
  accel.delay_min = settings_on_wiggle.min_delay + random(0, 400);
  // Override the acceleration settings
  //accel.set_target(accel.steps_to_target, 
  //  settings_on_wiggle.min_delay + random(0, 200), 
  //  settings_on_wiggle.max_delay, settings_on_wiggle.accel);

  if(nxt != position)
    dprintf(LOG_DEBUG, "%d: wiggle next: from %ld to %ld\n", idx, position, nxt);
}

void stepper_t::choose_next_sweep() {
  accel.set_pause_ms(1000);

  if (position == pos_end) {
    //dprintf(LOG_DEBUG, "%d: Sweep at open. Reversing\n", idx);
    set_target(-DEFAULT_MAX_STEPS);
  } else {
    set_target(pos_end);
    //dprintf(LOG_DEBUG, "%d: Sweep at close. Cooling off\n", idx);
    set_onoff(STEPPER_OFF);
  }

  accel.accel_0 = 0.00002;
  accel.delay_min = DELAY_MIN;
}

void stepper_t::set_forward(bool f) {
  if (forward != f)
    accel.set_pause_min(); //enforce short pause before reversing

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

  if(!steps_to_mid_point)
    steps_to_mid_point = 1; // Avoid divide by zero

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
  bench.next();
  log();

  if(state == STEP_INIT || state == STEP_OPEN) {
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

  if (!accel.is_ready())
    return;

  set_onoff(STEPPER_ON);

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

void stepper_t::trigger_close() {
  switch(state) {
    case STEP_INIT:
    case STEP_TRIGGERED_INIT:
    case STEP_GRAB:
    case STEP_GRAB_WIGGLE:
    case STEP_CLOSE:
      return;
  }

  how_wiggly = 2; // random(1, 3);
  choose_next(STEP_TRIGGERED_INIT);
}

void stepper_t::log() {
  uint32_t now = millis();
  if (debug_level >= LOG_DEBUG && now - last_log > 1000) {
    uint32_t us = micros();
    last_log = now;

    const char* state_str = nullptr;
    switch (state) {
      case STEP_INIT: state_str = "init"; break;
      case STEP_WIGGLE_START: state_str = "wiggle_start"; break;
      case STEP_WIGGLE_END: state_str = "wiggle_end"; break;
      case STEP_CLOSE: state_str = "close"; break;
      case STEP_OPEN: state_str = "open"; break;
      case STEP_RELAX: state_str = "relax"; break;
      case STEP_TRIGGERED_INIT: state_str = "triggered_init"; break;
      case STEP_GRAB: state_str = "grab"; break;
      case STEP_GRAB_WIGGLE: state_str = "grab_wiggle"; break;
      case STEP_DETANGLE: state_str = "detangle"; break;
      case STEP_SWEEP: state_str = "sweep"; break;
      default: state_str = "unknown"; break;
    }

    dprintf(LOG_DEBUG, "%d: Position: %ld, Target: %ld, State: %s, Fwd/Back: %d, "
        "Accel Delay: %ld, A0: %f, is ready: %d !(%lu && %lu)\n",
      idx, position, pos_tgt == -INT_MAX ? -99999 : pos_tgt,
      state_str, forward, 
      accel.delay_current, accel.accel_0, accel.is_ready(),
      us - accel.t_last_update < accel.t_pause_for,
      us - accel.t_last_update < accel.delay_current);

    bench.log_force();
  }
}