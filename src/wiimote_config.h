#ifndef WIIMOTE_CONFIG_H_
#define WIIMOTE_CONFIG_H_

struct WiimoteCredentials {
    const char *username;
    const char *password;
    const char *bearerToken;
};

struct WiimoteConfig {
    bool wifiEnabled;
    WiimoteCredentials credentials;
};

#endif  // WIIMOTE_CONFIG_H_