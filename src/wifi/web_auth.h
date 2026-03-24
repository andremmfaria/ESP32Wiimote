#ifndef WEB_AUTH_H_
#define WEB_AUTH_H_

#include <cstddef>
#include <cstdint>

/**
 * Web Authentication Module
 *
 * Provides Bearer token and Basic auth validation for REST API routes.
 */

// ===== Auth Result Enum =====

enum class WebAuthResult {
    Ok,                  // Authentication successful
    MissingHeader,       // Authorization header not present
    MalformedHeader,     // Header format invalid (e.g., missing space in "Bearer <token>")
    UnsupportedScheme,   // Scheme is neither Bearer nor Basic
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

// ===== Basic Auth =====

/**
 * Validate Basic auth from Authorization header.
 *
 * Expects: Authorization: Basic <base64-encoded-credentials>
 * The base64 payload decodes to "username:password".
 *
 * @param authHeaderValue Full value of Authorization header (e.g., "Basic dXNlcjpwYXNz")
 * @param creds Runtime credentials to validate against
 * @return WebAuthResult indicating success or failure reason
 */
WebAuthResult webAuthValidateBasic(const char *authHeaderValue, const char *wifiApiToken);

// ===== Combined Auth Entrypoint =====

/**
 * Validate Authorization header against either Bearer or Basic scheme.
 *
 * @param authHeaderValue Full value of Authorization header
 *                        (e.g., "Bearer token123" or "Basic dXNlcjpwYXNz")
 * @param creds Runtime credentials to validate against
 * @return WebAuthResult; Ok if either scheme succeeds, error otherwise
 */
WebAuthResult webAuthValidate(const char *authHeaderValue, const char *wifiApiToken);

#endif  // WEB_AUTH_H_
