#ifndef WIIMOTE_CONFIG_H_
#define WIIMOTE_CONFIG_H_

struct WiimoteNetworkCredentials {
    const char *ssid;
    const char *password;

    WiimoteNetworkCredentials(const char *ssidIn = nullptr, const char *passwordIn = nullptr)
        : ssid(ssidIn), password(passwordIn) {}
};

struct WiimoteConfig {
    bool wifiEnabled;
    const char *serialPrivilegedToken;
    const char *wifiApiToken;
    WiimoteNetworkCredentials network;

    WiimoteConfig() : wifiEnabled(false), serialPrivilegedToken(nullptr), wifiApiToken(nullptr) {}

    WiimoteConfig(bool wifiEnabledIn,
                  const char *serialPrivilegedTokenIn,
                  const char *wifiApiTokenIn,
                  WiimoteNetworkCredentials networkIn = WiimoteNetworkCredentials())
        : wifiEnabled(wifiEnabledIn)
        , serialPrivilegedToken(serialPrivilegedTokenIn)
        , wifiApiToken(wifiApiTokenIn)
        , network(networkIn) {}
};

#endif  // WIIMOTE_CONFIG_H_