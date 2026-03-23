#include "web_auth.h"

#include <cstring>

// ===== Phase 4 Hardcoded Credentials =====

static constexpr const char *kHardcodedBearerToken = "esp32wiimote_bearer_token_v1";
static constexpr const char *kHardcodedBasicUsername = "admin";
static constexpr const char *kHardcodedBasicPassword = "password";

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

WebAuthResult webAuthValidateBearer(const char *authHeaderValue) {
    if (authHeaderValue == nullptr) {
        return WebAuthResult::MissingHeader;
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

    // Validate against hardcoded token
    if (std::strcmp(token, kHardcodedBearerToken) != 0) {
        return WebAuthResult::InvalidCredentials;
    }

    return WebAuthResult::Ok;
}

// ===== Basic Auth Validation =====

WebAuthResult webAuthValidateBasic(const char *authHeaderValue) {
    if (authHeaderValue == nullptr) {
        return WebAuthResult::MissingHeader;
    }

    // Trim leading whitespace
    authHeaderValue = trimLeading(authHeaderValue);

    // Check "Basic " prefix
    if (!startsWithCaseInsensitive(authHeaderValue, "basic ")) {
        return WebAuthResult::UnsupportedScheme;
    }

    // Extract base64 part after "Basic "
    const char *base64 = authHeaderValue + 6;  // strlen("Basic ") == 6
    base64 = trimLeading(base64);

    // Decode base64 credentials (max space for "username:password")
    char decoded[64];
    size_t decodedLen = base64Decode(base64, decoded, sizeof(decoded) - 1);
    if (decodedLen == 0) {
        return WebAuthResult::MalformedHeader;
    }
    decoded[decodedLen] = '\0';

    // Parse "username:password" format
    const char *colonPtr = std::strchr(decoded, ':');
    if (colonPtr == nullptr) {
        return WebAuthResult::MalformedHeader;
    }

    size_t usernameLen = static_cast<size_t>(colonPtr - decoded);
    const char *password = colonPtr + 1;

    // Validate username and password lengths
    size_t expectedUsernameLen = std::strlen(kHardcodedBasicUsername);
    size_t expectedPasswordLen = std::strlen(kHardcodedBasicPassword);

    if (usernameLen != expectedUsernameLen) {
        return WebAuthResult::InvalidCredentials;
    }

    // Compare username (case-sensitive)
    if (std::strncmp(decoded, kHardcodedBasicUsername, usernameLen) != 0) {
        return WebAuthResult::InvalidCredentials;
    }

    // Compare password (case-sensitive)
    if (std::strcmp(password, kHardcodedBasicPassword) != 0) {
        return WebAuthResult::InvalidCredentials;
    }

    return WebAuthResult::Ok;
}

// ===== Combined Auth Entrypoint =====

WebAuthResult webAuthValidate(const char *authHeaderValue) {
    if (authHeaderValue == nullptr) {
        return WebAuthResult::MissingHeader;
    }

    // Trim and check for empty
    const char *trimmed = trimLeading(authHeaderValue);
    if (*trimmed == 0) {
        return WebAuthResult::MissingHeader;
    }

    // Try Bearer first
    WebAuthResult bearerResult = webAuthValidateBearer(authHeaderValue);
    if (bearerResult == WebAuthResult::Ok) {
        return WebAuthResult::Ok;
    }

    // Try Basic second
    WebAuthResult basicResult = webAuthValidateBasic(authHeaderValue);
    if (basicResult == WebAuthResult::Ok) {
        return WebAuthResult::Ok;
    }

    // If both schemes were unsupported, return the first unsupported error
    if (bearerResult == WebAuthResult::UnsupportedScheme &&
        basicResult == WebAuthResult::UnsupportedScheme) {
        return WebAuthResult::UnsupportedScheme;
    }

    // Otherwise, return a malformed/invalid error (prioritize invalid credentials)
    if (bearerResult == WebAuthResult::InvalidCredentials ||
        basicResult == WebAuthResult::InvalidCredentials) {
        return WebAuthResult::InvalidCredentials;
    }

    // Fallback: malformed
    return WebAuthResult::MalformedHeader;
}
