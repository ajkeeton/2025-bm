#include "splat.h"
#include "common.h"

CRGB _leds1[NUM_LEDS1];
CRGB _leds2[NUM_LEDS2];
CRGB _leds3[NUM_LEDS3];
CRGB _leds4[NUM_LEDS4];
CRGB _leds5[NUM_LEDS5];
CRGB _leds6[NUM_LEDS6];
CRGB _leds7[NUM_LEDS7];
CRGB _leds8[NUM_LEDS8];

void splat_t::init() {
    nstrips = 0;
    strips[nstrips++].init(_leds1, NUM_LEDS1);
    strips[nstrips++].init(_leds2, NUM_LEDS2);
    strips[nstrips++].init(_leds3, NUM_LEDS3);
    strips[nstrips++].init(_leds4, NUM_LEDS4);
    strips[nstrips++].init(_leds5, NUM_LEDS5);
    strips[nstrips++].init(_leds6, NUM_LEDS6);
    strips[nstrips++].init(_leds7, NUM_LEDS7);
    strips[nstrips++].init(_leds8, NUM_LEDS8);

    FastLED.addLeds<LED_TYPE, LED_PIN1, COLOR_ORDER>(_leds1, NUM_LEDS1).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, LED_PIN2, COLOR_ORDER>(_leds2, NUM_LEDS2).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, LED_PIN3, COLOR_ORDER>(_leds3, NUM_LEDS3).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, LED_PIN4, COLOR_ORDER>(_leds4, NUM_LEDS4).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, LED_PIN5, COLOR_ORDER>(_leds5, NUM_LEDS5).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, LED_PIN6, COLOR_ORDER>(_leds6, NUM_LEDS6).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, LED_PIN7, COLOR_ORDER>(_leds7, NUM_LEDS7).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, LED_PIN7, COLOR_ORDER>(_leds8, NUM_LEDS8).setCorrection(TypicalLEDStrip);

    FastLED.setBrightness(255);
    FastLED.setMaxPowerInVoltsAndMilliamps(12,8500);
    FastLED.clear();

    // Each strip has an ID to distinguish them in log messages
    for(int i=0; i<num_strips(); i++) {
        strips[i].id = i;
    }

    #ifdef TEST_WHITE_ONLY
    // NOTE: with the current LED strips, all black pulls around 0.35A, all white 1.3A
    while(true) {
        for(int i=0; i<num_strips(); i++) {
            strips[i].go_white();
        }
        FastLED.show();
        FastLED.delay(1000);
    }
    #endif
}

void splat_t::strips_next() {
  bool update = false;
  for(int i=0; i<nstrips; i++) {
    update |= strips[i].step();
  }

  if(update)
    FastLED.show();
  FastLED.delay(1);
}

void splat_t::next_core_0() {
  bench_led_calcs.start();
  for(int i=0; i<nstrips; i++) {
    strips[i].background_update();
  }
  bench_led_calcs.end();
}

void splat_t::next_core_1() {
  log_info();

  #if 0
  // Check for any commands from the garden server
  bench_wifi.start();
  recv_msg_t msg;
  if(wifi.recv_pop(msg)) {
      switch(msg.type) {
          case PROTO_PULSE:
              Serial.printf("Handling pulse message! color: %lu, fade: %u, spread: %u, delay: %lu\n",
                  msg.pulse.color, msg.pulse.fade, msg.pulse.spread, msg.pulse.delay);
              for(int i=0; i<nstrips; i++) {
                  strips[i].handle_remote_pulse(
                      msg.pulse.color, 
                      msg.pulse.fade, 
                      msg.pulse.spread, 
                      msg.pulse.delay);
              }
              break;
          case PROTO_STATE_UPDATE:
              // XXX There might not be reason to track the pattern_idx in the state
              state.handle_remote_update(msg.state.pattern_idx, msg.state.score);
              break;
          case PROTO_PIR_TRIGGERED:
              Serial.printf("Handling remote PIR message! %u\n", msg.pir.placeholder);
              state.on_remote_pir();
              break;
          case PROTO_SLEEPY_TIME:
              Serial.println("Received sleep command from server");
              break;
          default:
              Serial.printf("Ignoring message with type: 0x%X\n", msg.type);
              break;
      }
  }
  bench_wifi.end();
  #endif

  bench_led_push.start();
  strips_next();
  bench_led_push.end();
}

void splat_t::log_info() {
  EVERY_N_MILLISECONDS(500) {
    Serial.printf("Benchmarks (%d strips): led_calcs=%lu, led_push=%lu\n",
         nstrips, bench_led_calcs.elapsed(), bench_led_push.elapsed());
  }
}