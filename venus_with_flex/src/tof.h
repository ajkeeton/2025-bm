#include "Adafruit_VL53L0X.h"
#include "Wire.h"

/* 
    NEVER COULD GET INITIALIZATION TO PASS. 
    locks up on .begin(...)
*/
class tof_t {
public:
    tof_t() {
        lox = Adafruit_VL53L0X();
    }

    bool init() {
        Wire.setSDA(16);
        Wire.setSCL(17);
        Wire.begin();
        
        i2c_scan();

        return false;

        Serial.println("Initializing VL53L0X...");

        if (!lox.begin(VL53L0X_I2C_ADDR, true, &Wire, Adafruit_VL53L0X::VL53L0X_SENSE_LONG_RANGE)) {
            Serial.println("Failed to boot VL53L0X");
            return false;
        }

        Serial.println("VL53L0X online!");

        initialized = true;
        lox.startRangeContinuous();
        return true;
    }

    void next() {
        return;

        if (initialized && lox.isRangeComplete()) {
            dist = lox.readRange();
            Serial.printf("Distance: %u mm\n", dist);
        }
    }   

    void i2c_scan() {
        Serial.println("Scanning I2C bus...");

        for (uint8_t address = 1; address < 127; address++) {
            Wire.beginTransmission(address);
            uint8_t error = Wire.endTransmission();

            if (error == 0) {
            Serial.printf("I2C device found at address 0x%02X\n", address);
            } else if (error == 4) {
            Serial.printf("Unknown error at address 0x%02X\n", address);
            }
        }

        Serial.println("I2C scan complete.");
    }

    Adafruit_VL53L0X lox;
    bool initialized = false;
    uint32_t dist = 0;
};