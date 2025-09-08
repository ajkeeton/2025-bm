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

#define DEBOUNCE_MIN 3

class debounce_n_t {
public:
  bool update(uint16_t val, uint16_t min) {
    // the sonar is returning 0 every other loop...
    if(!val) {
        if(nzero++ > 0) {
            n = 0;
        }
        
        return false;
    }
    else {
        nzero = 0;
    }

    if(val > min) {
        n = 0;
        return false;
    }

    if(n > DEBOUNCE_MIN) {
        return true;
    }

    n++;

    return false;
  }

private:
  int n = 0;

  int nzero = 0;
};