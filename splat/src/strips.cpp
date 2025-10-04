#include "common.h"
#include "strips.h"

void strip_t::init(CRGB *l, uint16_t nleds) {
  leds = l;
  num_leds = nleds;

  for(int i=0; i<num_leds; i++) {
    leds[i] = CRGB::Black;
  }

  layer_white.init(nleds, false);
  layer_colored_glow.init(nleds, true);
  layer_waves.init(nleds, false);
  layer_tracers.init(nleds, false);
  layer_transition.init(nleds, false);
  
  // These are the LED animations themselves
  rainbow.init(nleds);
  waves.init(layer_waves.targets, nleds);
  wave_pulse.init(layer_waves.targets, nleds);
  blobs.init(layer_waves.targets, nleds);
  blob_asc.init(layer_waves.targets, nleds);
  // white.init(layer_white.targets, nleds);

  if(triggered) {
    delete []triggered;
  }
  triggered = new bool[num_leds];

  for(int i=0; i<num_leds; i++) {
    triggered[i] = false;
  }

  tracer_sens.init(layer_tracers.targets, nleds);
  reverse_pulse.init(layer_tracers.targets, nleds);

  for(int i=0; i<n_rand_tracers; i++) {
    tracers_rand[i].init(layer_tracers.targets, nleds, 0);
  }
}

bool strip_t::step() {
  uint32_t now = millis();
  uint32_t lapsed = now - t_last_update;
  if(lapsed < 1)
    return false;

  // Serial.printf("Strip %d step, pattern %d\n", id, pattern);
  t_last_update = now;
  mutex_enter_blocking(&mtx);

  for(int i=0; i<num_leds; i++) {
    #warning figure out best to additively blend layers
      //layer_colored_glow.blend(leds[i], i, 255);
      //layer_waves.blend(leds[i], i, 20);
      //transition.blend(leds[i], i, 80);
      
      layer_waves.blend(leds[i], i, 255);
      layer_tracers.blend(leds[i], i, 200);
    //}
    //leds[i] = layer_tracers.leds[i];
    //nblend(leds[i], layer_tracers.leds[i], 255);
  }
  
  layer_colored_glow.fade(5);
  mutex_exit(&mtx);

  return true;
}

void strip_t::fade_all(uint16_t amount) {
  // XXX amount is a percentage with an extra 0
  // The fade amount is out of 255, and we update fast enough that we want to 
  // scale that back
  amount = map(amount, 0, 1000, 0, 20);

  for(int i=0; i<num_leds; i++) {
    layer_colored_glow.fade(amount);
    layer_waves.fade(amount);
    //layer_tracers.fade(amount);
  }
}

// Update the layers, but don't push to the LEDs
void strip_t::background_update() {
  uint16_t amount = 0;
  uint16_t global_activity = 10;

  mutex_enter_blocking(&mtx);
  // XXX Future: Only update when in a state a pattern that might use it
  // if(need_rainbow...
  //rainbow.update();
  tracer_sens.step();
  reverse_pulse.step();

  blobs.step(global_activity);
  do_basic_ripples(global_activity);

  mutex_exit(&mtx);
}

void strip_t::do_basic_ripples(uint16_t activity) {
  for(int i=0; i<n_rand_tracers; i++)
    tracers_rand[i].step(activity);

  // high activity increases fade
  uint16_t fade = 5;
  //uint16_t fade = map(activity, 0, 100, 10, 30);
  //layer_tracers.fade(fade);
}

void strip_t::do_high_energy_ripples(uint16_t activity) {
  for(int i=0; i<n_rand_tracers; i++)
    tracers_rand[i].step(activity);

  uint16_t fade = map(activity, 0, 100, 10, 30);
  layer_tracers.fade(fade);
}

// Force all LEDs white, skip the layers. Mostly for debugging
void strip_t::force_white() {
  for(int i=0; i<num_leds; i++) {
    leds[i] = CRGB::White;
  }
}
