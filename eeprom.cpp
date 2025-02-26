//
// Created by nischal on 2/26/25.
//

#include "eeprom.h"

// Constructor
Eeprom::Eeprom(i2c_inst_t *i2cPort, uint8_t eepromAddr)
    : i2cPort(i2cPort), eepromAddr(eepromAddr) {
}

// Read a single byte from EEPROM
int Eeprom::singleRead(const int addr) const {
    const uint8_t addrToWrite[2] = {
        static_cast<uint8_t>((addr >> 8) & 0xFF),
        static_cast<uint8_t>(addr & 0xFF)
    };

    uint8_t returnedData[1];

    i2c_write_blocking(i2cPort, eepromAddr, addrToWrite, 2, true);
    sleep_ms(10); // Necessary delay for EEPROM read operation
    int result = i2c_read_blocking(i2cPort, eepromAddr, returnedData, 1, false);

    if (result > 0) {
        return returnedData[0];
    }
    return -1;
}


// Write a single byte dynamically (only writes if data is different)
void Eeprom::singleWrite(const int addr, const int data) const {
    int existingData = singleRead(addr);
    if (existingData == data) {
        return; // Skip writing if the value is already the same
    }

    const uint8_t dataToWrite[3] = {
        static_cast<uint8_t>((addr >> 8) & 0xFF),
        static_cast<uint8_t>(addr & 0xFF),
        static_cast<uint8_t>(data)
    };

    if (i2c_write_blocking(i2cPort, eepromAddr, dataToWrite, 3, false) > 0) {
    } else {
    }
}
