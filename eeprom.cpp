//
// Created by nischal on 2/26/25.
//

#include "eeprom.h"
#include <iostream>
#include <ostream>


// Eeprom eeprom(I2C_PORT, EEPROM_ADDR); // Define the eeprom object


// Constructor
Eeprom::Eeprom(i2c_inst_t *i2cPort, uint8_t eepromAddr)
    : i2cPort(i2cPort), eepromAddr(eepromAddr) {
}

int Eeprom::singleRead(const int addr, const bool largerBit) const {
    const uint8_t addrToWrite[2] = {
        static_cast<uint8_t>((addr >> 8) & 0xFF),   // High byte of the address
        static_cast<uint8_t>(addr & 0xFF)            // Low byte of the address
    };

    uint8_t returnedData[2]; // For 16-bit data, we need 2 bytes

    i2c_write_blocking(i2cPort, eepromAddr, addrToWrite, 2, true);
    sleep_ms(10);  // Allow time for EEPROM to process the read request
    int result = i2c_read_blocking(i2cPort, eepromAddr, returnedData, largerBit ? 2 : 1, false);

    if (result == (largerBit ? 2 : 1)) {  // Check if the correct number of bytes was read
        if (largerBit) {
            // Combine the two 8-bit values into a single 16-bit value
            int combinedData = (returnedData[0] << 8) | returnedData[1];
            std::cout << "Read 16-bit data: " << combinedData << std::endl;
            return combinedData;
        } else {
            std::cout << "Read 8-bit data: " << static_cast<int>(returnedData[0]) << std::endl;
            return returnedData[0];  // Return the 8-bit value
        }
    } else {
        std::cout << "Data unable to be read" << std::endl;
        return -1;  // Indicate error
    }
}

void Eeprom::singleWrite(const int addr, const int data, const bool is16Bit) const {
    if (is16Bit) {
        // Split 16-bit value into two 8-bit values
        const uint8_t dataToWrite[4] = {
            static_cast<uint8_t>((addr >> 8) & 0xFF),   // High byte of the address
            static_cast<uint8_t>(addr & 0xFF),          // Low byte of the address
            static_cast<uint8_t>((data >> 8) & 0xFF),   // High byte of the data (16-bit)
            static_cast<uint8_t>(data & 0xFF)           // Low byte of the data (16-bit)
        };

        int result = i2c_write_blocking(i2cPort, eepromAddr, dataToWrite, 4, false);
        if (result > 0) {
            std::cout << "Write 16-bit data done" << std::endl;
        } else {
            std::cout << "Write Failed" << std::endl;
        }
    } else {
        // For 8-bit data
        const uint8_t dataToWrite[3] = {
            static_cast<uint8_t>((addr >> 8) & 0xFF),  // High byte of the address
            static_cast<uint8_t>(addr & 0xFF),         // Low byte of the address
            static_cast<uint8_t>(data)                 // Data (8-bit)
        };

        int result = i2c_write_blocking(i2cPort, eepromAddr, dataToWrite, 3, false);
        if (result > 0) {
            std::cout << "Write 8-bit data done" << std::endl;
        } else {
            std::cout << "Write Failed" << std::endl;
        }
    }
    sleep_ms(10); // Give EEPROM time to write
}
