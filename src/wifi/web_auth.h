#ifndef WEB_AUTH_H_
#define WEB_AUTH_H_

#include <cstddef>
#include <cstdint>

/**
 * Web Authentication Module
 *
 * Provides Bearer token validation for REST API routes.
 */

// ===== Auth Result Enum =====

enum class WebAuthResult {
    Ok,                  // Authentication successful
    MissingHeader,       // Authorization header not present
    MalformedHeader,     // Header format invalid (e.g., missing space in "Bearer <token>")
    UnsupportedScheme,   // Scheme is not Bearer
    InvalidCredentials,  // Credentials do not match hardcoded values
};

// ===== Bearer Token =====

/**
 * Validate Bearer token from Authorization header.
 *
 * Expects: Authorization: Bearer <token>
 *
 * @param authHeaderValue Full value of Authorization header (e.g., "Bearer abc123")
 * @param creds Runtime credentials to validate against
 * @return WebAuthResult indicating success or failure reason
 */
WebAuthResult webAuthValidateBearer(const char *authHeaderValue, const char *wifiApiToken);

// ===== Combined Auth Entrypoint =====

/**
 * Validate Authorization header against the Bearer scheme.
 *
 * @param authHeaderValue Full value of Authorization header
 *                        (e.g., "Bearer token123")
 * @param creds Runtime credentials to validate against
 * @return WebAuthResult; Ok if the Bearer token matches, error otherwise
 */
WebAuthResult webAuthValidate(const char *authHeaderValue, const char *wifiApiToken);

#endif  // WEB_AUTH_H_
