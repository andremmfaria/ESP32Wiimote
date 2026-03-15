// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "hci_utils.h"

#include <stdio.h>

#define FORMAT_HEX_MAX_BYTES 30
static char formatHexBuffer[(FORMAT_HEX_MAX_BYTES * 3) + 4];

char *format2Hex(uint8_t *data, uint16_t len) {
    for (uint16_t i = 0; i < len && i < FORMAT_HEX_MAX_BYTES; i++) {
        const size_t kOffset = static_cast<size_t>(3U) * static_cast<size_t>(i);
        (void)snprintf(formatHexBuffer + kOffset, 4, "%02X ", data[i]);
        formatHexBuffer[kOffset + 3] = '\0';
    }

    if (FORMAT_HEX_MAX_BYTES < len) {
        const size_t kOffset = static_cast<size_t>(3U) * static_cast<size_t>(FORMAT_HEX_MAX_BYTES);
        (void)snprintf(formatHexBuffer + kOffset, 4, "...");
        formatHexBuffer[kOffset + 3] = '\0';
    }

    return formatHexBuffer;
}
