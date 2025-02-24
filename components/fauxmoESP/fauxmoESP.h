#ifndef FAUXMO_ESP_H
#define FAUXMO_ESP_H

extern "C"
{
#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "lwip/sockets.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

#include <vector>
#include <string>
#include <functional>

#define FAUXMO_UDP_MULTICAST_IP "239.255.255.250"
#define FAUXMO_UDP_MULTICAST_PORT 80 // ✅ Required for Alexa Gen 3
#define FAUXMO_TCP_PORT 1901
#define FAUXMO_TCP_MAX_CLIENTS 10

typedef std::function<void(unsigned char, const char *, bool, unsigned char)> TSetStateCallback;

struct FauxmoDevice
{
    std::string name;
    bool state;
    unsigned char value;
    std::string uniqueid;
};

class FauxmoESP
{
public:
    FauxmoESP();
    ~FauxmoESP();

    void enable(bool enable);
    unsigned char addDevice(const std::string &device_name);
    bool setState(unsigned char id, bool state, unsigned char value);
    bool setState(const std::string &device_name, bool state, unsigned char value);
    void handle();

private:
    bool _enabled = false;
    std::vector<FauxmoDevice> _devices;
    int udp_socket;
    int udp_socket_legacy;

    void _setupUDP();
    void _handleUDP();
    void _sendUDPResponse();
};

#endif // FAUXMO_ESP_H
