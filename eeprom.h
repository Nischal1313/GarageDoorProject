//
// Created by nischal on 2/26/25.
//
#ifndef EEPROM_H
#define EEPROM_H

#include "hardware/i2c.h"

class Eeprom {
private:
    i2c_inst_t *i2cPort; // I2C port instance
    uint8_t eepromAddr;  // EEPROM device address

public:
    // Constructor
    Eeprom(i2c_inst_t *i2cPort, uint8_t eepromAddr);

    // Read a single byte from EEPROM
    [[nodiscard]] int singleRead(int addr) const;

    // Write a single byte dynamically (only writes if data is different)
    void singleWrite(int addr, int data) const;
};

#endif // EEPROM_H
