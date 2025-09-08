#include "leds.h"

DEFINE_GRADIENT_PALETTE( BCY ) {
    0,   0,  0,255,
   63,   0, 55,255,
  127,   0,255,255,
  191,  42,255, 45,
  255, 255,255,  0};

static CRGBPalette16 palette_wave = BCY;
CRGB _leds[NUM_LEDS];

void leds_t::init() {
    leds = _leds;
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>
        (leds, num_leds).setCorrection(TypicalLEDStrip);
    FastLED.setMaxPowerInVoltsAndMilliamps(12,2000);
    FastLED.clear();

    set_brightness(255);

    set_all_leds(CRGB::Black);

    layer_colored_glow.init(num_leds, true);
    layer_waves.init(num_leds, false);
    layer_tracers.init(num_leds, false);

    rainbow.init(num_leds);
    waves.init(layer_waves.targets, num_leds);
    wave_pulse.init(layer_waves.targets, num_leds);
    blobs.num_blobs = 2;
    blobs.init(layer_waves.targets, num_leds);
    t1.init(layer_tracers.targets, num_leds, 120);
    t2.init(layer_tracers.targets, num_leds, 0);

    sparkles.init(layer_tracers.targets, num_leds);
}

void leds_t::step() {
    uint32_t now = millis();

    if (now - t_last_update < 5) {
        FastLED.delay(1);
        return;
    }
    
    handle_glow();

    //Serial.println("leds_t::step() - updating LEDs");
    t_last_update = now;

    for(int i=0; i<num_leds; i++) {
      layer_colored_glow.blend(leds[i], i, 200);
      layer_waves.blend(leds[i], i, 20);
      layer_tracers.blend(leds[i], i, 80);
    }
    
    layer_colored_glow.fade(2);
    layer_tracers.fade(10);
    
    FastLED.show();
}

void leds_t::background_update() {
    rainbow.update();
    //t1.step(random8(1, 5));
    //t2.step(random8(0, 2));

    //sparkles.step();

    //wave_pulse.step(100);
    // blobs.step(10);
}

void leds_t::trigger(uint16_t pct) {
    trigger_pct = (trigger_pct * 19 + pct) / 20;
    
    if(!trigger_time) {
        trigger_time = millis();
        triggered = true;
    }
}

void leds_t::handle_glow() {    
    if(trigger_pct < 1) {
        triggered = false;
        trigger_pct = 0;
        trigger_time = 0;
        return;
    }
    #ifdef LED_ON_PIR
    else {
        trigger_pct *= 0.99;
    }

    uint32_t now = millis();
    uint32_t age = now - trigger_time;
    if(age > 3500)
        age = 3500;
    uint8_t brightness = map(age, 0, 3500, 0, 255);

    CRGB color = rainbow.get(0, brightness);
    uint16_t max_spread = num_leds * 0.025;
    uint16_t spread = map(trigger_pct, 0, 100, 0, max_spread);
    spread = constrain(spread, 1, max_spread);  

    //Serial.printf("leds_t::handle_glow - age: %u, brightness: %u, spread: %u\n", age, brightness, spread);

    for(int i=0; i<spread; i++) {
        int j = LED_AT_TIP - i;
        int k = LED_AT_TIP + i;

        if(j >= 0) {
            layer_colored_glow.set(j, color, 250);
        }
        if(k < num_leds) {
            layer_colored_glow.set(k, color, 250);
        }
    }

    layer_colored_glow.blur(100);

    #else
    uint32_t now = millis();
    uint32_t age = now - trigger_time;
    if(age > 5000)
        age = 5000;

    uint8_t brightness = 200; // map(age, 0, 5000, 20, 255);
    CRGB color = rainbow.get(0, brightness);

    uint16_t max_spread = num_leds * 0.5;
    uint16_t spread = map(trigger_pct, 0, 100, 0, max_spread);
    spread = constrain(spread, 1, max_spread);  

   // Serial.printf("leds_t::handle_glow - age: %u, brightness: %u, spread: %u\n", age, brightness, spread);

    for(int i=0; i<spread; i++) {
        int j = LED_AT_TIP - i;
        int k = LED_AT_TIP + i;
        uint8_t b = 255 - map(i, 0, spread, 0, brightness);

        if(j >= 0) {
            layer_colored_glow.set(j, color, 250);
        }
        if(k < num_leds) {
            layer_colored_glow.set(k, color, 250);
        }
    }

    layer_colored_glow.blur(100);
    #endif
}

void leds_t::handle_pir() {
    trigger_pct = 100;
    if(!trigger_time) {
        trigger_time = millis();
        triggered = true;
    }
}

void leds_t::handle_sonar() {
    trigger_pct = 100;
    if(!trigger_time) {
        trigger_time = millis();
        triggered = true;
    }
}
    
#if 0
void leds_t::trigger(uint16_t pct) {
    Serial.printf("leds_t::trigger - %d%\n", pct);

    trigger_pct = (trigger_pct * 9 + pct) / 10;

    uint16_t spread = map(trigger_pct, 0, 100, 0, num_leds * 0.8);

    uint32_t now = millis();
    uint32_t age = 0,
             brightness = 0;
    if(!triggered) {
        trigger_time = millis();
        triggered = true;
    }
    else {
        age = now - trigger_time;
        if(age > 5000)
            age = 5000;
        brightness = map(age, 0, 5000, 0, 255);
    }

    if(spread > 100)
        spread = 100;

  //if(percent) 
  //  Serial.printf("strip_t::on_trigger led: %u, percent: %u, duration: %u, spread: %u\n", 
  //    led, percent, duration, spread);

  for(int i=0; i<spread; i++) {
    int j = LED_AT_TIP - i;
    int k = LED_AT_TIP + i;
    uint8_t b = 255 - map(i, 0, spread, 0, brightness);

    if(j >= 0) {
        CRGB color = rainbow.get(j, b);
      layer_colored_glow.set(j, color, 250);
    }
    if(k < num_leds) {
      CRGB color = rainbow.get(k, b);
      layer_colored_glow.set(k, color, 250);
    }
  }

  layer_colored_glow.blur(100);
}
#endif