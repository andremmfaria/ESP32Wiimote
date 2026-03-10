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
        sprintf(formatHexBuffer + (3 * i), "%02X ", data[i]);
        formatHexBuffer[(3 * i) + 3] = '\0';
    }

    if (FORMAT_HEX_MAX_BYTES < len) {
        sprintf(formatHexBuffer + (3 * FORMAT_HEX_MAX_BYTES), "...");
        formatHexBuffer[(3 * FORMAT_HEX_MAX_BYTES) + 3] = '\0';
    }

    return formatHexBuffer;
}
