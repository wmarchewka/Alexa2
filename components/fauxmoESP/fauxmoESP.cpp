#include "fauxmoESP.h"
#include <cstring>
#include <lwip/inet.h>

static const char *TAG = "FAUXMO";

FauxmoESP::FauxmoESP()
{
    ESP_LOGI(TAG, "Initializing FauxmoESP...");
    _setupUDP();
}

FauxmoESP::~FauxmoESP()
{
    close(udp_socket);
    _devices.clear();
}

void FauxmoESP::enable(bool enable)
{
    _enabled = enable;
    ESP_LOGI(TAG, "FauxmoESP %s", enable ? "enabled" : "disabled");
}

// void FauxmoESP::_setupUDP()
// {
//     ESP_LOGI(TAG, "Setting up UDP sockets on ports 80 & 1900...");

//     udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
//     if (udp_socket < 0)
//     {
//         ESP_LOGE(TAG, "Failed to create UDP socket for port 80, error: %d", errno);
//         return;
//     }

//     udp_socket_legacy = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
//     if (udp_socket_legacy < 0)
//     {
//         ESP_LOGE(TAG, "Failed to create UDP socket for port 1900, error: %d", errno);
//         return;
//     }

//     int reuse = 1;
//     setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
//     setsockopt(udp_socket_legacy, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

//     // ✅ Bind to port 80 (Newer Alexa)
//     struct sockaddr_in addr80;
//     memset(&addr80, 0, sizeof(addr80));
//     addr80.sin_family = AF_INET;
//     addr80.sin_port = htons(80);
//     addr80.sin_addr.s_addr = htonl(INADDR_ANY);

//     if (bind(udp_socket, (struct sockaddr *)&addr80, sizeof(addr80)) < 0)
//     {
//         ESP_LOGE(TAG, "Failed to bind UDP socket to port 80, error: %d", errno);
//         close(udp_socket);
//         return;
//     }

//     // ✅ Bind to port 1900 (Older Alexa)
//     struct sockaddr_in addr1900;
//     memset(&addr1900, 0, sizeof(addr1900));
//     addr1900.sin_family = AF_INET;
//     addr1900.sin_port = htons(1900);
//     addr1900.sin_addr.s_addr = htonl(INADDR_ANY);

//     if (bind(udp_socket_legacy, (struct sockaddr *)&addr1900, sizeof(addr1900)) < 0)
//     {
//         ESP_LOGE(TAG, "Failed to bind UDP socket to port 1900, error: %d", errno);
//         close(udp_socket_legacy);
//         return;
//     }

//     // ✅ Join multicast group for both ports
//     struct ip_mreq mreq;
//     mreq.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
//     mreq.imr_interface.s_addr = htonl(INADDR_ANY);

//     if (setsockopt(udp_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
//     {
//         ESP_LOGE(TAG, "Failed to join multicast group on port 80, error: %d", errno);
//     }
//     else
//     {
//         ESP_LOGI(TAG, "✅ Successfully joined SSDP multicast group on port 80.");
//     }

//     if (setsockopt(udp_socket_legacy, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
//     {
//         ESP_LOGE(TAG, "Failed to join multicast group on port 1900, error: %d", errno);
//     }
//     else
//     {
//         ESP_LOGI(TAG, "✅ Successfully joined SSDP multicast group on port 1900.");
//     }
// }

void FauxmoESP::_setupUDP()
{
    ESP_LOGI(TAG, "Setting up UDP sockets on ports 80 & 1900...");

    udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    udp_socket_legacy = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    int reuse = 1;
    setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    setsockopt(udp_socket_legacy, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // ✅ Bind to port 80 (Newer Alexa)
    struct sockaddr_in addr80 = {};
    addr80.sin_family = AF_INET;
    addr80.sin_port = htons(80);
    addr80.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(udp_socket, (struct sockaddr *)&addr80, sizeof(addr80));

    // ✅ Bind to port 1900 (Older Alexa)
    struct sockaddr_in addr1900 = {};
    addr1900.sin_family = AF_INET;
    addr1900.sin_port = htons(1900);
    addr1900.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(udp_socket_legacy, (struct sockaddr *)&addr1900, sizeof(addr1900));

    // Assign multicast TTL (set separately from normal interface TTL)
    uint8_t ttl = MULTICAST_TTL;
    setsockopt(udp_socket, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(uint8_t));
    if (err < 0) {
        ESP_LOGE(V4TAG, "Failed to set IP_MULTICAST_TTL. Error %d", errno);
        goto err;
    }

    // ✅ Join multicast group on both ports
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    setsockopt(udp_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    setsockopt(udp_socket_legacy, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    ESP_LOGI(TAG, "✅ Successfully joined SSDP multicast group on ports 80 & 1900.");
}


void FauxmoESP::_handleUDP()
{
    ESP_LOGI(TAG, "Handling UDP...");
    
    if (!_enabled)
        return;

    char buffer[512];
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    // ✅ Log packets received on Port 80 (Newer Alexa)
    int len = recvfrom(udp_socket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&sender_addr, &sender_len);
    if (len > 0)
    {
        buffer[len] = '\0';
        ESP_LOGI(TAG, "🔹 UDP Packet Received on Port 80 from %s:%d -> [%s]",
                 inet_ntoa(sender_addr.sin_addr), ntohs(sender_addr.sin_port), buffer);
    }

    // ✅ Log packets received on Port 1900 (Older Alexa)
    len = recvfrom(udp_socket_legacy, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&sender_addr, &sender_len);
    if (len > 0)
    {
        buffer[len] = '\0';
        ESP_LOGI(TAG, "🔹 UDP Packet Received on Port 1900 from %s:%d -> [%s]",
                 inet_ntoa(sender_addr.sin_addr), ntohs(sender_addr.sin_port), buffer);
    }
}

// void FauxmoESP::_handleUDP()
// {
//     if (!_enabled)
//         return;

//     char buffer[512];
//     struct sockaddr_in sender_addr;
//     socklen_t sender_len = sizeof(sender_addr);

//     // ✅ Check port 80 (Newer Alexa)
//     int len = recvfrom(udp_socket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&sender_addr, &sender_len);
//     if (len > 0)
//     {
//         buffer[len] = '\0';
//         ESP_LOGI(TAG, "🔹 Received UDP (Port 80) from %s:%d -> %s",
//                  inet_ntoa(sender_addr.sin_addr),
//                  ntohs(sender_addr.sin_port),
//                  buffer);
//         if (strstr(buffer, "M-SEARCH"))
//         {
//             ESP_LOGI(TAG, "✅ Alexa SSDP Discovery Request Detected on Port 80.");
//             _sendUDPResponse();
//         }
//     }

//     // ✅ Check port 1900 (Older Alexa)
//     len = recvfrom(udp_socket_legacy, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&sender_addr, &sender_len);
//     if (len > 0)
//     {
//         buffer[len] = '\0';
//         ESP_LOGI(TAG, "🔹 Received UDP (Port 1900) from %s:%d -> %s",
//                  inet_ntoa(sender_addr.sin_addr),
//                  ntohs(sender_addr.sin_port),
//                  buffer);
//         if (strstr(buffer, "M-SEARCH"))
//         {
//             ESP_LOGI(TAG, "✅ Alexa SSDP Discovery Request Detected on Port 1900.");
//             _sendUDPResponse();
//         }
//     }
// }

void FauxmoESP::_sendUDPResponse()
{
    if (!_enabled)
        return;

    struct sockaddr_in sender_addr;
    memset(&sender_addr, 0, sizeof(sender_addr));
    sender_addr.sin_family = AF_INET;
    sender_addr.sin_port = htons(80);
    sender_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    char response[512];
    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\n"
             "CACHE-CONTROL: max-age=100\r\n"
             "EXT:\r\n"
             "LOCATION: http://%s:80/setup.xml\r\n"
             "SERVER: Unspecified, UPnP/1.0, Unspecified\r\n"
             "ST: urn:Belkin:device:controllee:1\r\n"
             "USN: uuid:Socket-1_0-221438K0101769::urn:Belkin:device:controllee:1\r\n"
             "Content-Length: 0\r\n"
             "\r\n",
             "192.168.1.100"); // Replace with actual ESP32 IP

    int sent_bytes = sendto(udp_socket, response, strlen(response), 0, (struct sockaddr *)&sender_addr, sizeof(sender_addr));

    if (sent_bytes > 0)
    {
        ESP_LOGI(TAG, "✅ Sent SSDP Response to Alexa at %s", inet_ntoa(sender_addr.sin_addr));
    }
    else
    {
        ESP_LOGE(TAG, "❌ Failed to send response, error: %d", errno);
    }
}

void FauxmoESP::handle()
{
    if (_enabled)
    {
        _handleUDP();
    }
}

unsigned char FauxmoESP::addDevice(const std::string &device_name)
{
    FauxmoDevice device;
    device.name = device_name;
    device.state = false;
    device.value = 0;
    device.uniqueid = "00:00:" + std::to_string(_devices.size());

    _devices.push_back(device);
    ESP_LOGI(TAG, "Device added: %s", device_name.c_str());
    return _devices.size() - 1;
}
