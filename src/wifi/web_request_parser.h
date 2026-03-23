#ifndef WEB_REQUEST_PARSER_H
#define WEB_REQUEST_PARSER_H

#include <cstddef>
#include <cstdint>

/**
 * Web Request Parser
 *
 * Parses bounded JSON command bodies from REST API requests.
 * No dynamic allocation; uses fixed-size buffers.
 */

// ===== Parse Result Enum =====

enum class WebRequestParseResult {
    Ok,                    // Valid command parsed
    BodyTooLarge,          // Request body exceeds max size
    MalformedJson,         // JSON syntax error
    MissingRequiredField,  // Required field not found
    InvalidFieldType,      // Field has unexpected type (e.g., expected string got number)
};

// ===== Request Limits =====

static constexpr size_t kWebRequestMaxBodySize = 1024;  // Max request body in bytes
static constexpr size_t kWebRequestMaxValueSize = 256;  // Max string value length
static constexpr size_t kWebRequestMaxFieldCount = 16;  // Max JSON fields

// ===== Parsed Command Structure =====

struct WebParsedCommand {
    // Command verb (e.g., "scan_start", "set_led")
    char verb[64];

    // Key-value pairs extracted from JSON body
    struct Field {
        char key[64];
        char value[kWebRequestMaxValueSize];
    };

    Field fields[kWebRequestMaxFieldCount];
    size_t fieldCount;

    WebParsedCommand() : fieldCount(0) { verb[0] = '\0'; }
};

// ===== Parser Interface =====

/**
 * Parse a JSON command body.
 *
 * Expected format (minimal example):
 * {
 *   "command": "scan_start",
 *   "duration": "5"
 * }
 *
 * The "command" field is required and becomes the verb.
 * Other fields are extracted as key-value pairs.
 *
 * @param body Raw JSON body bytes
 * @param bodyLen Length of body in bytes
 * @param cmd Output parsed command (populated on success)
 * @return WebRequestParseResult indicating success or failure reason
 */
WebRequestParseResult webRequestParse(const char *body, size_t bodyLen, WebParsedCommand *cmd);

#endif  // WEB_REQUEST_PARSER_H
