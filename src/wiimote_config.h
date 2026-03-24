#ifndef WIIMOTE_CONFIG_H
#define WIIMOTE_CONFIG_H

struct WiimoteNetworkCredentials {
    const char *ssid;
    const char *password;

    WiimoteNetworkCredentials(const char *ssidIn = nullptr, const char *passwordIn = nullptr)
        : ssid(ssidIn), password(passwordIn) {}
};

#endif  // WIIMOTE_CONFIG_H_