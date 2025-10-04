#pragma once

#define WADS_V2A_BOARD
#include <map>
#include "common/common.h"
#include "common/patterns.h"

//#define TEST_WHITE_ONLY
//#define FIND_SENS 1
//#define LED_TEST 1

#define MIN_TRIG_FOR_STATE_CHANGE 2
#define STATE_FALL_TIMEOUT 60000 // this many ms inactive and we transition down

#define T_REMOTE_SENSOR_UPDATE_DELAY 5*1000 // ms to wait before sending sensor updates

#define MAX_RIPPLES_RAND 4

#define LED_PIN1    15
#define LED_PIN2    14
#define LED_PIN3    13
#define LED_PIN4    12
#define LED_PIN5    11
#define LED_PIN6    10
#define LED_PIN7    9
#define LED_PIN8    8

#define NUM_LEDS1   144 
#define NUM_LEDS2   144 
#define NUM_LEDS3   144
#define NUM_LEDS4   144
#define NUM_LEDS5   144
#define NUM_LEDS6   144
#define NUM_LEDS7   144
#define NUM_LEDS8   144

#define MAX_BRIGHT  255
#define LED_TYPE    WS2812
#define COLOR_ORDER RGB

#define DEF_HUE    225
#define DEF_SAT    255
#define DEF_BRIGHT 50

// For convenience with the state tracking
#define MAX_STRIPS 16
#define MAX_STRIP_LENGTH 144
#define MAX_MUX_IN 16
#define MUX_PIN_PIR 6

#define IN_BUTTON A2
#define IN_DIP A1

#define BUTTON_SENS_IN_PIN 4
