#include "web_auth.h"

#include <cstring>

// ===== Helper: Base64 Decode =====

/**
 * Minimal base64 decode for Basic auth credentials.
 *
 * Decodes in-place up to outMaxLen bytes.
 * Returns the number of decoded bytes.
 *
 * Does NOT validate base64 format strictly; assumes valid input.
 */
static size_t base64Decode(const char *encoded, char *out, size_t outMaxLen) {
    static constexpr const char *kBase64Chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    if (encoded == nullptr || out == nullptr || outMaxLen == 0) {
        return 0;
    }

    size_t outIdx = 0;
    int bits = 0;
    int bitCount = 0;

    for (const char *p = encoded; *p != 0; ++p) {
        // Stop at padding or invalid chars
        if (*p == '=') {
            break;
        }

        // Find base64 char value
        int val = -1;
        for (int i = 0; i < 64; ++i) {
            if (kBase64Chars[i] == *p) {
                val = i;
                break;
            }
        }

        if (val < 0) {
            break;  // Invalid character
        }

        bits = (bits << 6) | val;
        bitCount += 6;

        if (bitCount >= 8) {
            bitCount -= 8;
            if (outIdx >= outMaxLen) {
                break;  // Output buffer full
            }
            out[outIdx++] = static_cast<char>((bits >> bitCount) & 0xFF);
        }
    }

    return outIdx;
}

// ===== Helper: Trim Leading Whitespace =====

static const char *trimLeading(const char *str) {
    if (str == nullptr) {
        return str;
    }
    while (*str == ' ' || *str == '\t') {
        ++str;
    }
    return str;
}

// ===== Helper: Check String Prefix =====

static bool startsWithCaseInsensitive(const char *str, const char *prefix) {
    if (str == nullptr || prefix == nullptr) {
        return false;
    }
    while (*prefix != 0) {
        char s = *str;
        char p = *prefix;
        // Convert to lowercase for comparison
        if (s >= 'A' && s <= 'Z') {
            s = static_cast<char>(s - 'A' + 'a');
        }
        if (p >= 'A' && p <= 'Z') {
            p = static_cast<char>(p - 'A' + 'a');
        }
        if (s != p) {
            return false;
        }
        ++str;
        ++prefix;
    }
    return true;
}

// ===== Bearer Token Validation =====

WebAuthResult webAuthValidateBearer(const char *authHeaderValue, const char *wifiApiToken) {
    if (authHeaderValue == nullptr) {
        return WebAuthResult::MissingHeader;
    }
    if (wifiApiToken == nullptr) {
        return WebAuthResult::InvalidCredentials;
    }

    // Trim leading whitespace
    authHeaderValue = trimLeading(authHeaderValue);

    // Check "Bearer " prefix
    if (!startsWithCaseInsensitive(authHeaderValue, "bearer ")) {
        return WebAuthResult::UnsupportedScheme;
    }

    // Extract token after "Bearer "
    const char *token = authHeaderValue + 7;  // strlen("Bearer ") == 7
    token = trimLeading(token);

    if (std::strcmp(token, wifiApiToken) != 0) {
        return WebAuthResult::InvalidCredentials;
    }

    return WebAuthResult::Ok;
}

// ===== Basic Auth Validation =====

WebAuthResult webAuthValidateBasic(const char *authHeaderValue, const char * /*wifiApiToken*/) {
    if (authHeaderValue == nullptr) {
        return WebAuthResult::MissingHeader;
    }
    return WebAuthResult::UnsupportedScheme;
}

// ===== Combined Auth Entrypoint =====

WebAuthResult webAuthValidate(const char *authHeaderValue, const char *wifiApiToken) {
    if (authHeaderValue == nullptr) {
        return WebAuthResult::MissingHeader;
    }

    // Trim and check for empty
    const char *trimmed = trimLeading(authHeaderValue);
    if (*trimmed == 0) {
        return WebAuthResult::MissingHeader;
    }

    // Try Bearer first
    WebAuthResult bearerResult = webAuthValidateBearer(authHeaderValue, wifiApiToken);
    if (bearerResult == WebAuthResult::Ok) {
        return WebAuthResult::Ok;
    }

    return bearerResult;
}
