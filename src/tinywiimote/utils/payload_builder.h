// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef PAYLOAD_BUILDER_H_
#define PAYLOAD_BUILDER_H_

#include <stdint.h>
#include <string.h>

/**
 * PayloadBuilder - Helper class for building binary payloads
 *
 * Eliminates repetitive posi++ patterns and provides a clean API
 * for constructing HCI/L2CAP/Wiimote protocol packets.
 *
 * Example usage:
 *   uint8_t buffer[64];
 *   PayloadBuilder pb(buffer, sizeof(buffer));
 *   pb.append(0xA2);
 *   pb.append(0x11);
 *   pb.append(leds << 4);
 *   sender->send(buffer, pb.length());
 */
class PayloadBuilder {
   private:
    uint8_t *_buffer;
    uint16_t _position{0};
    uint16_t _capacity;

   public:
    /**
     * Construct a PayloadBuilder for a given buffer
     * @param buf Pointer to buffer
     * @param cap Buffer capacity
     */
    PayloadBuilder(uint8_t *buf, uint16_t cap) : _buffer(buf), _capacity(cap) {}

    /**
     * Append a single byte
     * @param val Byte value
     */
    void append(uint8_t val) {
        if (_position < _capacity) {
            _buffer[_position++] = val;
        }
    }

    /**
     * Append 16-bit value in little-endian format (low byte first)
     * @param val 16-bit value
     */
    void appendU16LE(uint16_t val) {
        append((uint8_t)(val & 0xFF));
        append((uint8_t)(val >> 8));
    }

    /**
     * Append 16-bit value in big-endian format (high byte first)
     * @param val 16-bit value
     */
    void appendU16BE(uint16_t val) {
        append((uint8_t)(val >> 8));
        append((uint8_t)(val & 0xFF));
    }

    /**
     * Append 24-bit value in big-endian format (used for Wiimote addresses)
     * @param val 24-bit value (only lower 24 bits used)
     */
    void appendU24BE(uint32_t val) {
        append((uint8_t)((val >> 16) & 0xFF));
        append((uint8_t)((val >> 8) & 0xFF));
        append((uint8_t)((val) & 0xFF));
    }

    /**
     * Append array of bytes
     * @param data Pointer to source data
     * @param len Number of bytes to append
     */
    void appendBytes(const uint8_t *data, uint16_t len) {
        for (uint16_t i = 0; i < len && _position < _capacity; i++) {
            _buffer[_position++] = data[i];
        }
    }

    /**
     * Append zero bytes (padding)
     * @param count Number of zero bytes to append
     */
    void appendZeros(uint16_t count) {
        for (uint16_t i = 0; i < count && _position < _capacity; i++) {
            _buffer[_position++] = 0x00;
        }
    }

    /**
     * Zero-fill a region and advance position
     * Unlike appendZeros, this uses memset for efficiency
     * @param count Number of bytes to zero-fill
     */
    void reserveZeroed(uint16_t count) {
        if (_position + count <= _capacity) {
            memset(&_buffer[_position], 0, count);
            _position += count;
        }
    }

    /**
     * Get current payload length
     * @return Number of bytes written
     */
    uint16_t length() const { return _position; }

    /**
     * Get pointer to buffer
     * @return Buffer pointer
     */
    uint8_t *data() const { return _buffer; }
};

#endif  // PAYLOAD_BUILDER_H_
