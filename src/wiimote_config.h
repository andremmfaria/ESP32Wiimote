#ifndef WIIMOTE_CONFIG_H_
#define WIIMOTE_CONFIG_H_

struct WiimoteCredentials {
    const char *username;
    const char *password;
    const char *bearerToken;
};

struct WiimoteNetworkCredentials {
    const char *ssid;
    const char *password;
};

struct WiimoteConfig {
    bool wifiEnabled;
    WiimoteCredentials credentials;
    WiimoteNetworkCredentials network;
};

#endif  // WIIMOTE_CONFIG_H_