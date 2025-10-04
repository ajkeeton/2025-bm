#include "stepper.h"

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

  td /= 5; // make it more manageable

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

  return delay_current;
}