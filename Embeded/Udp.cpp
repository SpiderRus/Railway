#include "esp_compiler.h"
#include "HardwareSerial.h"
#include <udp/udp.h>
#include <Arduino.h>

void IRAM_ATTR UDPSender::flushBuffer() noexcept {
    if (likely(bufferOffset > 0)) {
        IPAddress address;
        uint16_t port;

        if (likely(udpIpCallbackFunc(&address, &port))) {
            if (unlikely(broadcast))
                asyncUDP.broadcastTo(sendBuffer, bufferOffset, port);
            else
                asyncUDP.writeTo(sendBuffer, bufferOffset, address, port);
        }

        bufferOffset = 0;
    }
}

bool IRAM_ATTR UDPSender::pushToBuffer(UDP_MESSAGE_DATA &data) noexcept {
    auto offsetData = bufferOffset + 2;

    if (likely(offsetData < sizeof(sendBuffer))) {
        auto length = data.callback(data.args, sendBuffer + offsetData, sizeof(sendBuffer) - offsetData);

        if (likely(length >= 0)) {
            if (likely(length > 0)) {
                *(uint16_t*) (sendBuffer + bufferOffset) = (uint16_t) length;
                bufferOffset = offsetData + length;
            }

            return true;
        }
    }

    return false;
}

void IRAM_ATTR UDPSender::task() noexcept {
    UDP_MESSAGE_DATA data;

    do {
        if (receiveQueueItem(&data)) {
            pushToBuffer(data);

            while (receiveQueueItem(&data, 0)) {
                while (!pushToBuffer(data))
                    flushBuffer();
            }

            flushBuffer();
        }
    } while (true);
}

bool IRAM_ATTR UDPSender::sendFromISR(UdpCallbackFunc callback, void *args) noexcept {
    UDP_MESSAGE_DATA data = { callback, args };

    return xQueueSend(queue, &data, 0) == pdPASS;
}

typedef std::function<void(IPAddress& remoteIP, uint8_t messageType, uint8_t *buffer, size_t length, void *args)> UdpPacketFunc;

void parsePacket(AsyncUDPPacket &packet, UdpPacketFunc callback, void *arg) noexcept {
    uint8_t *data = packet.data();
    size_t length = packet.length();
    IPAddress remoteIp = packet.remoteIP();

    while (likely(length >= sizeof(uint16_t))) {
        length -= sizeof(uint16_t);
        const size_t messageLen = std::min((size_t) (*(uint16_t*) data), length);
        data += sizeof(uint16_t);

        if (likely(messageLen > 0)) {
            callback(remoteIp, *data, data + 1, messageLen - 1, arg);
            length -= messageLen;
            data += messageLen;
        }
    }
}
