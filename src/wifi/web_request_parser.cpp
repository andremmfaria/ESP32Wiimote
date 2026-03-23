#include "web_request_parser.h"

#include <cstring>

// ===== JSON State Machine Parser =====

namespace {

enum class JsonTokenType {
    ObjectStart,  // {
    ObjectEnd,    // }
    String,       // "..."
    Colon,        // :
    Comma,        // ,
    Eof,
    Error,
};

struct JsonToken {
    JsonTokenType type;
    char value[kWebRequestMaxValueSize];
    size_t valueLen;
};

class JsonParser {
   public:
    JsonParser(const char *data, size_t len) : data_(data), len_(len) {}

    JsonToken next() {
        skipWhitespace();

        if (pos_ >= len_) {
            return {JsonTokenType::Eof, "", 0};
        }

        char ch = data_[pos_];

        switch (ch) {
            case '{':
                ++pos_;
                return {JsonTokenType::ObjectStart, "", 0};
            case '}':
                ++pos_;
                return {JsonTokenType::ObjectEnd, "", 0};
            case ':':
                ++pos_;
                return {JsonTokenType::Colon, "", 0};
            case ',':
                ++pos_;
                return {JsonTokenType::Comma, "", 0};
            case '"':
                return parseString();
            default:
                return {JsonTokenType::Error, "", 0};
        }
    }

   private:
    const char *data_;
    size_t len_;
    size_t pos_{0};

    void skipWhitespace() {
        while (pos_ < len_ && (data_[pos_] == ' ' || data_[pos_] == '\t' || data_[pos_] == '\n' ||
                               data_[pos_] == '\r')) {
            ++pos_;
        }
    }

    JsonToken parseString() {
        if (data_[pos_] != '"') {
            return {JsonTokenType::Error, "", 0};
        }

        ++pos_;  // Skip opening quote
        JsonToken token{JsonTokenType::String, "", 0};

        while (pos_ < len_ && token.valueLen < kWebRequestMaxValueSize - 1) {
            char ch = data_[pos_];

            if (ch == '"') {
                // Found closing quote
                token.value[token.valueLen] = '\0';
                ++pos_;
                return token;
            }

            if (ch == '\\') {
                // Basic escape handling
                ++pos_;
                if (pos_ >= len_) {
                    return {JsonTokenType::Error, "", 0};
                }
                char escaped = data_[pos_];
                // For simplicity, just include the escaped char as-is
                // (not handling \u escapes etc.)
                if (escaped == 'n') {
                    token.value[token.valueLen++] = '\n';
                } else if (escaped == 't') {
                    token.value[token.valueLen++] = '\t';
                } else if (escaped == 'r') {
                    token.value[token.valueLen++] = '\r';
                } else {
                    // For '"', '\\', '/', and unknown escapes, include as-is
                    token.value[token.valueLen++] = escaped;
                }
                ++pos_;
            } else {
                token.value[token.valueLen++] = ch;
                ++pos_;
            }
        }

        // Unterminated string or value too long
        return {JsonTokenType::Error, "", 0};
    }
};

}  // namespace

// ===== Parser Implementation =====

WebRequestParseResult webRequestParse(const char *body, size_t bodyLen, WebParsedCommand *cmd) {
    if (body == nullptr || cmd == nullptr) {
        return WebRequestParseResult::MalformedJson;
    }

    if (bodyLen == 0 || bodyLen > kWebRequestMaxBodySize) {
        return WebRequestParseResult::BodyTooLarge;
    }

    JsonParser parser(body, bodyLen);

    // Expect opening {
    JsonToken token = parser.next();
    if (token.type != JsonTokenType::ObjectStart) {
        return WebRequestParseResult::MalformedJson;
    }

    bool foundCommand = false;
    cmd->fieldCount = 0;

    // Parse key-value pairs
    while (true) {
        token = parser.next();

        // Check for end of object
        if (token.type == JsonTokenType::ObjectEnd) {
            break;
        }

        // Expect key string
        if (token.type != JsonTokenType::String) {
            return WebRequestParseResult::MalformedJson;
        }

        // Copy key
        char currentKey[64];
        std::strncpy(currentKey, token.value, sizeof(currentKey) - 1);
        currentKey[sizeof(currentKey) - 1] = '\0';

        // Expect colon
        token = parser.next();
        if (token.type != JsonTokenType::Colon) {
            return WebRequestParseResult::MalformedJson;
        }

        // Expect value (string)
        token = parser.next();
        if (token.type != JsonTokenType::String) {
            return WebRequestParseResult::InvalidFieldType;
        }

        // Handle "command" field specially
        if (std::strcmp(currentKey, "command") == 0) {
            std::strncpy(cmd->verb, token.value, sizeof(cmd->verb) - 1);
            cmd->verb[sizeof(cmd->verb) - 1] = '\0';
            foundCommand = true;
        } else {
            // Store as regular field
            if (cmd->fieldCount >= kWebRequestMaxFieldCount) {
                return WebRequestParseResult::BodyTooLarge;
            }

            std::strncpy(cmd->fields[cmd->fieldCount].key, currentKey,
                         sizeof(cmd->fields[cmd->fieldCount].key) - 1);
            cmd->fields[cmd->fieldCount].key[sizeof(cmd->fields[cmd->fieldCount].key) - 1] = '\0';

            std::strncpy(cmd->fields[cmd->fieldCount].value, token.value,
                         sizeof(cmd->fields[cmd->fieldCount].value) - 1);
            cmd->fields[cmd->fieldCount].value[sizeof(cmd->fields[cmd->fieldCount].value) - 1] =
                '\0';

            ++cmd->fieldCount;
        }

        // Expect comma or end of object
        token = parser.next();
        if (token.type == JsonTokenType::ObjectEnd) {
            break;
        }
        if (token.type == JsonTokenType::Comma) {
            // Continue to next field
            continue;
        }
        return WebRequestParseResult::MalformedJson;
    }

    // Verify command field was present
    if (!foundCommand || cmd->verb[0] == '\0') {
        return WebRequestParseResult::MissingRequiredField;
    }

    return WebRequestParseResult::Ok;
}
