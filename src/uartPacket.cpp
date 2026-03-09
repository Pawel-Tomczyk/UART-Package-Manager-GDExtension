#include "uartPacket.hpp"

uint8_t calculateChecksum(UartPacket *cmd)
{
    uint8_t checksum = 0;
    checksum ^= cmd->id;
    checksum ^= cmd->length;
    for (int i = 0; i < cmd->length; i++)
    {
        checksum ^= cmd->data[i];
    }
    return checksum;
}
unsigned int makePacket(UartPacket *cmd, uint8_t *packet)
{
    packet[0] = START_BYTE_1;
    packet[1] = START_BYTE_2;
    packet[2] = cmd->id;
    packet[3] = cmd->length;
    if (cmd->length > 255)
    {
        return 0; // error: length exceeds maximum allowed value
    }

    for (int i = 0; i < cmd->length; i++)
    {
        packet[4 + i] = cmd->data[i];
    }
    packet[4 + cmd->length] = calculateChecksum(cmd);
    return 5 + cmd->length; // total packet size is 5 bytes (2 start bytes, 1 id byte, 1 length byte, 1 checksum byte) + the length of the data
}

/*
    changes an int32_t to a byte array and vice versa, and the same for int16_t
    this is needed because the data is sent as bytes over UART, but we want to work with integers in the code
    ensures that the data is sent in big-endian format (most significant byte first)
*/
void int32ToBytes(int32_t value, uint8_t *bytes, uint8_t startIndex)
{
    bytes[startIndex + 0] = (value >> 24) & 0xFF;
    bytes[startIndex + 1] = (value >> 16) & 0xFF;
    bytes[startIndex + 2] = (value >> 8) & 0xFF;
    bytes[startIndex + 3] = value & 0xFF;
}

int32_t bytesToInt32(uint8_t *bytes, uint8_t startIndex)
{
    return (uint32_t(bytes[startIndex + 0]) << 24) |
           (uint32_t(bytes[startIndex + 1]) << 16) |
           (uint32_t(bytes[startIndex + 2]) << 8) |
           uint32_t(bytes[startIndex + 3]);
}

void int16ToBytes(int16_t value, uint8_t *bytes, uint8_t startIndex)
{
    bytes[startIndex + 0] = (value >> 8) & 0xFF;
    bytes[startIndex + 1] = value & 0xFF;
}

int16_t bytesToInt16(uint8_t *bytes, uint8_t startIndex)
{
    return (uint16_t(bytes[startIndex + 0]) << 8) |
           uint16_t(bytes[startIndex + 1]);
}
