#pragma once

#include <Wire.h>
#include <stdint.h>

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

#define LED_1 14
#define LED_2 15

/////////////////////////////////////////
// Following values are for the *mux* pins
// Flex sens
#define SENS_LIM_1 4
#define SENS_LIM_2 5
#define SENS_LIM_3 6
#define SENS_LIM_4 7

// Prox sensors
#define SENS_PIR_1 3
/////////////////////////////////////////

// Dip switches
#define INPUT_SWITCH_0 A1
#define INPUT_SWITCH_1 A2