#include "stepper.h"
#include <cmath>

int32_t accel_t::_constrain(int32_t x) {
  x = max(delay_min, x);
  return min(delay_max, x);
}

// So handy! https://www.desmos.com/calculator
int32_t accel_t::next() {
  count++;

  // Speed is in pulses/sec
  // Accel val is in pulses/sec**2
  // Use t_move_start to figure out what I current speed should be 
  // Convert pulse/sec back to delay
  
  uint32_t now = micros();
  float lapsed = now - t_move_started;

  // desired speed in pulse/sec
  float d = lapsed / accel_0;// + lapsed*lapsed / accel_1;

  // Serial.printf("Accel: %f/%ld = %f\n", d, steps_to_target, d / steps_to_target);

  d /= steps_to_target;

  // Serial.printf("       %ld - %f = %f\n", delay_current, d, delay_current - d);

  delay_current = max(delay_min, delay_init - d);
  delay_current = min(delay_max, delay_current);

  // Serial.printf("       returning: %u\n", ret);
  return delay_current;
}

// Uses a plateau function to acceleration
int32_t accel_t::next_plat() {
  count++;

  // total time lapsed since we started moving
  uint32_t now = micros();
  
  // Integer overflow will happen here around 70 minutes
  if(t_move_started > now)
    t_move_started = now; // XXX gross hack that's technically wrong

  float td = now - t_move_started;

  if(count == steps_to_target) {
    // We're at the midway point, use the time elapsed to determine time remaining
    t_move_started = micros();
    time_to_target = td;
    // Serial.printf("Decelerating. TTT: %lu\n", time_to_target);
    return delay_current;
  }

  float et = 0;

  if(count < steps_to_target)
    et = accel_0 * td; // the sweet spot seems to be between 0.00001 to 0.0001
  else
    et = accel_0 * (time_to_target - td);

  float mult = 1 - exp(-et);
  // Mult is now a number between 0 and 1 that will scale the delay

  //if(count < steps_to_target)
  //  mult = 1/mult;

  delay_current = _constrain(delay_max - mult * delay_max);
  //Serial.printf("Accel @ %d:\t%f: %f .... delay: %u\n", count, et, mult, delay_current);
  //Serial.printf("wtf: %lu - %lu = %lu\n", td, time_to_target, td-time_to_target);

  return delay_current;
}

float accel_t::windowed_steps_to_target() {
  if(window) {
    if(count < window)
      return window - count;
    if(count > steps_to_target - window)
      return count - (steps_to_target - window);
    // return count so we get a plateau in the middle
    return count;
  }
  return steps_to_target;
}

#if 0
int32_t accel_t::next_s() {
  count++;
  if (steps_to_target <= 0)
    return delay_current;

  // normalized progress 0..1 based on full travel
  float p = (float)count / (float)steps_to_target;
  if (p < 0.0f) p = 0.0f;
  if (p > 1.0f) p = 1.0f;

  // mirror progress so we get a bell: 0 at start, 1 at midpoint, 0 at end
  float t = (p <= 0.5f) ? (p * 2.0f) : ((1.0f - p) * 2.0f);

  // smoothstep / ease in-out on t -> gives smooth bell when mirrored
  float s = 3.0f * t * t - 2.0f * t * t * t;

  // bias the curve: >1 makes the approach to the ends slower (tune to taste)
  const float curve = 1.0f; // increase to slow approach more
  s = powf(s, curve);

  // map s (0..1) to delay: s==0 => delay_max (slow), s==1 => delay_min (fast)
  int32_t target = delay_max - (int32_t)(s * (float)(delay_max - delay_min));
  delay_current = _constrain(target);

  return delay_current;
}
#endif

int32_t accel_t::next_s() {
  count++;
  if (steps_to_target <= 0)
    return delay_current;

  float s = 0.0f;
  const float edge_steep = 0.5f; // <1 -> steeper edges (lower => steeper)

  // If a "window" is configured, use it to produce a true flat plateau
  // in the center (full speed). The ramps on either end use smoothstep.
  if (window > 0 && steps_to_target > 2 * window) {
    if (count >= window && count <= steps_to_target - window) {
      // center plateau
      s = 1.0f;
    } else {
      // ramp region length == window
      float t;
      if (count < window)
        t = (float)count / (float)window;
      else
        t = (float)(steps_to_target - count) / (float)window;
      if (t < 0.0f) t = 0.0f;
      if (t > 1.0f) t = 1.0f;
      // steepen edges by remapping t before smoothstep
      t = powf(t, edge_steep);
      // smoothstep ramp
      s = 3.0f * t * t - 2.0f * t * t * t;
    }
  } else {
    // No explicit window: mirror progress to make a bell shape
    float p = (float)count / (float)steps_to_target;
    if (p < 0.0f) p = 0.0f;
    if (p > 1.0f) p = 1.0f;
    float t = (p <= 0.5f) ? (p * 2.0f) : ((1.0f - p) * 2.0f);
    // steepen ramp edges
    t = powf(t, edge_steep);
    s = 3.0f * t * t - 2.0f * t * t * t;
  }

  // Additional flattening transform: raises center values toward 1
  // Increase flatness (>1.0) to widen/flatten the plateau.
  const float flatness = 1.0f;
  s = 1.0f - powf(1.0f - s, flatness);

  // Map s (0..1) to delay range: 0 -> delay_max (slow), 1 -> delay_min (fast)
  int32_t target = delay_max - (int32_t)(s * (float)(delay_max - delay_min));
  delay_current = _constrain(target);
  return delay_current;
}

#if 0
int32_t accel_t::next() {
  // Speed is in pulses/sec
  // Accel val is in pulses/sec**2
  // Use t_move_start to figure out what I current speed should be 
  // Convert pulse/sec back to delay
  
  uint32_t now = micros();
  float lapsed = now - t_move_started;

  // we're in micros, so make lapsed more manageable
  lapsed /= ACCEL_CONST_DIV;

  // desired speed in pulse/sec
  float d = lapsed / accel_0 + lapsed*lapsed / accel_1;

  Serial.printf("Accel: %f + %f = %f ... ", lapsed / accel_0, lapsed*lapsed / accel_1, d);

  if(state == ACCEL_UP) {
    d = -d;

  uint32_t ret = max(delay_min, delay_current + d);
  ret = min(delay_max, ret);
  Serial.printf("delay current: %u, returning: %u\n", delay_current, ret);
  return ret;
}
#endif

#if 0
int32_t accel_t::next() {
  #if 0
  if(step_counts == step_target)
    return delay_current + step_counts * step_size;

  step_counts++;

  //Serial.printf("Accel: %d * %f = %f ... delay current: %u tgt: %u\n ", 
  //  step_counts, step_size, step_counts * step_size, delay_current, step_target);

  return delay_current + step_counts * step_size;
  #endif
  
  // NOTE: intentionally doing the calc even when we pass the target point then clamping
  // simplifies some things with minimal performance impact

  step_counts++;

  int32_t ret = delay_current + step_counts * step_size;
  if(ret < 0 || ret < delay_min)  // the < 0 is due to wrap around and comparing int vs uint
    ret = delay_min;
  if(ret > delay_max)
    ret = delay_max;

  //Serial.printf("Accel: %d * %f = %f ... delay current: %u tgt: %u. ret: %ld\n ", 
  //  step_counts, step_size, step_counts * step_size, delay_current, step_target, ret);

  return ret;
}
#endif