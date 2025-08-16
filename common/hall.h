#pragma once
#include <stdint.h>
#include "common/mux.h"

extern mux_t mux;

class hall_t {
public:
    void init(uint8_t p) {
        pin = p;
    }
    bool is_triggered() {
        return mux.read_raw(pin) < 50;
    }

private:
    uint8_t pin;
};