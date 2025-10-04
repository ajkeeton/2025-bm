#pragma once

#include <Wire.h>
#include <stdint.h>

//#define BLOOM_TOP

//#define SENS_LOG_ONLY
//#define OPEN_CLOSE_ONLY
// #define SWEEP_ONLY

enum LOG_LEVEL {
  LOG_ERROR,
  LOG_INFO,
  LOG_DEBUG,
};

#define DEF_LOG LOG_INFO

#define DEF_SENS_THOLD 800

#define STEP_EN_1 2
#define STEP_PULSE_1 3
#define STEP_DIR_1 4

#define STEP_EN_2 5
#define STEP_PULSE_2 6
#define STEP_DIR_2 7
 
#define STEP_EN_3 8
#define STEP_PULSE_3 9
#define STEP_DIR_3 10

#define STEP_EN_4 11
#define STEP_PULSE_4 12
#define STEP_DIR_4 13

#define SIGNAL_IN_BLOOM1 13
#define SIGNAL_IN_BLOOM2 12

#define LED_1 14
#define LED_2 15

/////////////////////////////////////////
// Following values are for the *mux* pins
// Flex sens
#define SENS_LIMIT_1 7
#define SENS_LIMIT_2 6
#define SENS_LIMIT_3 5

#define SENS_REMOTE_1 4
#define SENS_REMOTE_2 3
/////////////////////////////////////////

// Dip switches
#define INPUT_SWITCH_0 A1
#define INPUT_SWITCH_1 A2