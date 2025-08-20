#pragma once

#include <Wire.h>
#include <stdint.h>

#define DEF_SENS_THOLD 800

#define LED_ON_PIR
// Whether we should use sonar or PIRs for bloom
#define BLOOM_ON_SONAR

// Prox sensors
#define SENS_TOF_1 4
#define SENS_PIR_1 5
/////////////////////////////////////////

// Dip switches
#define INPUT_SWITCH_0 A1
#define INPUT_SWITCH_1 A2