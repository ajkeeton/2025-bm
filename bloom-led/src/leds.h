#pragma once

#include "common/patterns.h"
#include <FastLED.h>

#define LED_PIN 15
#define LED_TYPE WS2812
#define COLOR_ORDER RGB
#define NUM_LEDS 144
#define LED_AT_TIP 135

class leds_t {
public:
    leds_t() : leds(nullptr), num_leds(NUM_LEDS) {}
    leds_t(CRGB* leds, int num_leds) : leds(leds), num_leds(num_leds) {}
    
    void init();

    void set_all_leds(const CRGB& color) {
        for (int i = 0; i < num_leds; ++i) {
            leds[i] = color;
        }
        FastLED.show();
    }

    void set_led(int index, const CRGB& color) {
        if (index >= 0 && index < num_leds) {
            leds[index] = color;
            FastLED.show();
        }
    }

    void clear() {
        fill_solid(leds, num_leds, CRGB::Black);
        FastLED.show();
    }

    void set_brightness(uint8_t brightness) {
        FastLED.setBrightness(brightness);
    }

    void show_rainbow() {
        for (int i = 0; i < num_leds; ++i) {
            //leds[i] = CHSV(state++, 255, 255); // CHSV((i * 255 / num_leds), 255, 255);
            leds[i] = blend(leds[i], CHSV(state++, 255, 255), 30);
        }
        FastLED.show();
    }

    void do_rainbow();
    void do_basic_ripples(uint16_t activity);
    void do_high_energy_ripples(uint16_t activity);
    void do_wave(uint8_t brightness, int refresh);
    void background_update();
    void step();
    void trigger(uint16_t dist);

private:
    int pattern_idx = 0;
    layer_t layer_waves,
            layer_colored_glow,
            layer_tracers;

    tracer_t t1, t2;
    animate_waves_t waves;
    wave_pulse_t wave_pulse;
    blobs_t blobs;
    rainbow_t rainbow;

    CRGB* leds = nullptr;
    int num_leds = NUM_LEDS;
    uint32_t t_last_update = 0;
    uint8_t state = 0;


    uint32_t trigger_time = 0;
    float trigger_pct = 0;
    bool triggered = false;
};