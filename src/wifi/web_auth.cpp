#include "web_auth.h"

#include <cstring>

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
    while (*prefix != 0) {
        char s = *str;
        const char p = *prefix;
        // Prefix is passed in lowercase by callers, so only normalize input.
        if (s >= 'A' && s <= 'Z') {
            s = static_cast<char>(s - 'A' + 'a');
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
