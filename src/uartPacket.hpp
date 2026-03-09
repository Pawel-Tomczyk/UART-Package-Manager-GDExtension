#pragma once

#include <cstdint>


#define START_BYTE_1 0xAB // start byte, used to identify the beginning of a packet, helps to synchronize the communication and avoid reading garbage data
#define START_BYTE_2 0xCD // second start byte, used to further ensure that we are reading a valid packet, reduces the chances of false positives when looking for the start of a packet

struct UartPacket
{
    uint8_t id;  // command id, used to identify the type of command
    uint8_t length;    // length of the data, used to know how many bytes to read for the data
    uint8_t data[256]; // data, used to store the actual data of the command, can be up to 256 bytes long (because the length is stored in a single byte, it can only represent values from 0 to 255)
    uint8_t checksum;  // checksum of the packet, used to verify the integrity of the data, ensures that the data has not been corrupted during transmission
};

/*
    calculates the checksum of a packet, used to verify the integrity of the data, ensures that the data has not been corrupted during transmission
    the checksum is calculated by xoring all the bytes of the packet (excluding the start bytes and the checksum byte itself)
    if the calculated checksum matches the checksum byte in the packet, then the data is considered valid
*/
uint8_t calculateChecksum(UartPacket *cmd);
unsigned int makePacket(UartPacket *cmd, uint8_t *packet);

/*
    changes an int32_t to a byte array and vice versa, and the same for int16_t
    this is needed because the data is sent as bytes over UART, but we want to work with integers in the code
    ensures that the data is sent in big-endian format (most significant byte first)
*/
void int32ToBytes(int32_t value, uint8_t *bytes, uint8_t startIndex = 0);
int32_t bytesToInt32(uint8_t *bytes, uint8_t startIndex = 0);
void int16ToBytes(int16_t value, uint8_t *bytes, uint8_t startIndex = 0);
int16_t bytesToInt16(uint8_t *bytes, uint8_t startIndex = 0);
