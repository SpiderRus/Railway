#ifndef __UDP_H
#define __UDP_H

#include "Arduino.h"
#include "IPAddress.h"
#include "freertos/projdefs.h"
#include <AsyncUDP.h>
#include <utils/Utils.h>
#include <utils/QueueWorker.h>

#define PACKET_TYPE_LOG 0
#define PACKET_TYPE_HANDSHAKE 2
#define PACKET_TYPE_HANDSHAKE_RESPONSE 3

typedef std::function<int16_t(void *args, uint8_t *buffer, size_t maxLen)> UdpCallbackFunc;

typedef std::function<bool(IPAddress *addressPtr, uint16_t *portPtr)> UdpIpCallbackFunc;

typedef struct UDP_MESSAGE_DATA_ {
    UdpCallbackFunc callback;
    void *args;
} UDP_MESSAGE_DATA;

class UDPSender : protected QueueWorkerBase {
public:
    UDPSender(AsyncUDP &asyncUDP, UdpIpCallbackFunc udpIpCallbackFunc, bool broadcast = false, unsigned int priority = UDP_TASK_PRIORITY, uint queueSize = 100) noexcept :
                    QueueWorkerBase(queueSize, "UDP_SEND", priority, sizeof(UDP_MESSAGE_DATA)),
                    asyncUDP(asyncUDP), udpIpCallbackFunc(udpIpCallbackFunc), broadcast(broadcast), bufferOffset(0) {
    }

    inline bool __attribute__((__always_inline__)) send(UdpCallbackFunc callback, void *args) noexcept {
        UDP_MESSAGE_DATA data = { callback, args };

        return sendQueueItem(&data);
    }

    bool IRAM_ATTR sendFromISR(UdpCallbackFunc callback, void *args) noexcept;

protected:
    void IRAM_ATTR task() noexcept override final;

private:
    void IRAM_ATTR flushBuffer() noexcept;

    bool IRAM_ATTR pushToBuffer(UDP_MESSAGE_DATA &data) noexcept;

    AsyncUDP &asyncUDP;
    const bool broadcast;
    UdpIpCallbackFunc udpIpCallbackFunc;

    size_t bufferOffset;
    uint8_t sendBuffer[CONFIG_TCP_MSS];
};


typedef std::function<void(IPAddress& remoteIP, uint8_t messageType, uint8_t *buffer, size_t length, void *args)> UdpPacketFunc;

extern void parsePacket(AsyncUDPPacket &packet, UdpPacketFunc callback, void *arg = nullptr) noexcept;

#endif // __UDP_H