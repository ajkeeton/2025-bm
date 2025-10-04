#pragma once

#include <TFLI2C.h>  // TFLuna-I2C Library v.0.2.0

class lidar_t {
public:
    void init() {
        Wire.setSDA(16);
        Wire.setSCL(17);
        Wire.begin();

        tf = TFLI2C();

        Serial.print("TF Luna: ");
        uint8_t ver[3];
        if( tf.Get_Firmware_Version(ver, TFL_DEF_ADR)) {
            Serial.print( ver[2]);
            Serial.print( ".");
            Serial.print( ver[1]);
            Serial.print( ".");
            Serial.println( ver[0]);
        }
        else {
            tf.printStatus();
            Serial.println();
        }
    }

    bool next() {
        uint32_t now = millis();

        if(now - t_last_read < 30)
            return true;
        t_last_read = now;

        if (tf.getData(distance, strength, temp, TFL_DEF_ADR)) {
            temp /= 100;
            distance *= 0.393701; // cm to inches
            return true;
        } else {
            distance = 0;
            return false;
        }
    }

    uint32_t t_last_read = 0;
    TFLI2C tf;
    int16_t distance;
    int16_t strength;
    int16_t temp;
};