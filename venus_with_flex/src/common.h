#pragma once

#include <Wire.h>
#include <stdint.h>

//#define SENS_LOG_ONLY
//#define OPEN_CLOSE_ONLY

enum LOG_LEVEL {
  LOG_ERROR,
  LOG_INFO,
  LOG_DEBUG,
};

#define DEFAULT_LOG LOG_INFO

#define DEF_SENS_THOLD 800

#ifdef USING_SINGLE_BOARD
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
#else
#define STEP_EN_1 1
#define STEP_PULSE_1 2
#define STEP_DIR_1 3

#define STEP_EN_2 4
#define STEP_PULSE_2 5
#define STEP_DIR_2 6
 
#define STEP_EN_3 7
#define STEP_PULSE_3 8
#define STEP_DIR_3 9
#endif

#define LED_1 14
#define LED_2 15

/////////////////////////////////////////
// Following values are for the *mux* pins
// Flex sens
#define SENS_FLEX_1 0
#define SENS_FLEX_2 1
#define SENS_FLEX_3 2

// Prox sensors
#define SENS_PROX_1 3
//#define SENS_TOF_1 4
#define SENS_PIR_1 5
/////////////////////////////////////////

// Dip switches
#define INPUT_SWITCH_0 A1
#define INPUT_SWITCH_1 A2

// ToF / Lidar ranges with current mux & v div
// ~.75 to 5.5 inches
#define SENS_TOF_MIN 15
#define SENS_TOF_MAX 130
#define SENS_TOF_PIN 3
