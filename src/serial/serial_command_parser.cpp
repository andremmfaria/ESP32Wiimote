#include "serial_command_parser.h"

#include <cstring>

SerialParseResult serialCommandParse(const char *line, SerialParsedCommand *out) {
    const size_t kLen = strlen(line);
    if (kLen > kSerialMaxLineLength) {
        return SerialParseResult::LineTooLong;
    }

    memcpy(out->buf, line, kLen + 1);
    out->tokenCount = 0;

    char *p = out->buf;
    bool overflow = false;

    while (*p != '\0') {
        // Skip whitespace
        while (*p == ' ' || *p == '\t' || *p == '\r') {
            ++p;
        }
        if (*p == '\0') {
            break;
        }

        if (out->tokenCount >= kSerialMaxTokens) {
            overflow = true;
            break;
        }

        out->tokens[out->tokenCount++] = p;

        // Advance past the token body
        while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\r') {
            ++p;
        }
        if (*p != '\0') {
            *p = '\0';
            ++p;
        }
    }

    if (out->tokenCount == 0) {
        return SerialParseResult::EmptyLine;
    }

    if (strcmp(out->tokens[0], "wm") != 0) {
        return SerialParseResult::NotACommand;
    }

    if (overflow) {
        return SerialParseResult::TooManyTokens;
    }

    return SerialParseResult::Ok;
}
