#ifndef ESP32_WIIMOTE_SERIAL_COMMAND_PARSER_H
#define ESP32_WIIMOTE_SERIAL_COMMAND_PARSER_H

#include <stdint.h>

// Maximum input line length accepted by the parser (bytes, excluding null terminator).
static constexpr uint8_t kSerialMaxLineLength = 128;

// Maximum number of whitespace-separated tokens per command line.
static constexpr uint8_t kSerialMaxTokens = 10;

enum class SerialParseResult : uint8_t {
    Ok,             // Parsed successfully; tokens[0] == "wm"
    EmptyLine,      // Line is empty or contains only whitespace
    NotACommand,    // First token is not "wm"
    LineTooLong,    // Line exceeds kSerialMaxLineLength bytes
    TooManyTokens,  // Token count exceeds kSerialMaxTokens
};

struct SerialParsedCommand {
    char buf[kSerialMaxLineLength + 1];  // Working copy of the input line
    const char *tokens[kSerialMaxTokens];
    uint8_t tokenCount;
};

// Parse a null-terminated input line into `out`.
// The input must not include the trailing newline character.
// On return, `out->tokens` point into `out->buf` (stable as long as `out` is alive).
SerialParseResult serialCommandParse(const char *line, SerialParsedCommand *out);

#endif  // ESP32_WIIMOTE_SERIAL_COMMAND_PARSER_H
