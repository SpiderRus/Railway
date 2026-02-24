#ifndef INC_UDP_HPP
#define INC_UDP_HPP

#include <AsyncUDP.h>
#include <lwip/udp.h>
#include "Utils.hpp"
#include "configuration.hpp"

#define UDP_ROOT_CODE       1
#define UDP_HANDSHAKE_CODE  2
#define UDP_LOG_CODE        3
#define UDP_MESSAGE_ARRAY   4
#define UDP_MESSAGE_SPEED   5
#define UDP_TIME_SYNC       6

#pragma pack(push, 1)

typedef struct _UDP_MESSAGE {
  uint16_t code;
} UDP_MESSAGE;

typedef struct _UDP_HANDSHAKE_MESSAGE : UDP_MESSAGE {
  uint32_t ip;
  char hostname[];
} UDP_HANDSHAKE_MESSAGE;

typedef struct _UDP_LOG_MESSAGE : UDP_MESSAGE {
  char level;
  uint64_t stamp;
  char data[]; // hostname, tag, msg
} UDP_LOG_MESSAGE;

typedef struct _UDP_ARRAY_MESSAGE_ITEM {
  uint16_t length;
  uint8_t data[];
} UDP_ARRAY_MESSAGE_ITEM;

typedef struct _UDP_ARRAY_MESSAGE : UDP_MESSAGE {
  uint16_t count;
  uint8_t data[];
} UDP_ARRAY_MESSAGE;

typedef struct _TIMED_MESSAGE : UDP_MESSAGE {
  uint64_t stamp;
  uint16_t hostnameLen;
  char hostname[sizeof(CONFIGURATION_DATA::hostname)];
} TIMED_MESSAGE;

typedef struct _SPEED_MESSAGE : public TIMED_MESSAGE {
  int16_t minPower;
  int16_t maxPower;
  int16_t power;

  uint16_t maxRps;
  uint16_t rps;

  int16_t minSpeed;
  int16_t maxSpeed;
  int16_t speed;
} SPEED_MESSAGE;

typedef struct _TIME_SYNC_MESSAGE : public TIMED_MESSAGE {
  uint64_t requested;
} TIME_SYNC_MESSAGE;

#pragma pack(pop)

//typedef std::function<void(const UDP_MESSAGE& message, size_t length)> UdpPacketHandlerFunction;

typedef void (*UdpPacketHandlerFunction)(const UDP_MESSAGE& message, size_t length);

typedef std::function<void(void *ptr, size_t maxLen)> UdpPacketWriter;

class Udp : protected AsyncUDP {
  public:
    // Udp(IPAddress gIp, uint16_t port, UdpPacketHandlerFunction h) : AsyncUDP(), 
    //     groupIp(gIp), groupAddr(IPADDR4_INIT((uint32_t)groupIp)), port(port) {
    //   handlers.add(h);
    // }

    inline __attribute__((__always_inline__)) Udp() noexcept {
    }

    bool begin(const IPAddress ip, uint16_t port, const IPAddress locIp = IPAddress(), const char *hostname = NULL) noexcept;

    void end() noexcept;

    size_t writeTo(size_t maxLen, UdpPacketWriter writer) noexcept {
      if (!started || !port)
        return 0;
        
      const uint32_t i = (uint32_t)ip;

      if (!i)
        return 0;

      ip_addr_t addr = IPADDR4_INIT(i);
      if (!_pcb && !connect(&addr, port))
         return 0;
    
      if (maxLen > CONFIG_TCP_MSS)
          maxLen = CONFIG_TCP_MSS;

      pbuf* pbt = pbuf_alloc(PBUF_TRANSPORT, maxLen, PBUF_RAM);
      if (pbt != NULL) {
          writer(pbt->payload, maxLen);
          
        
          err_t err = udp_sendto(_pcb, pbt, &addr, port);
          pbuf_free(pbt);
        
          return err < ERR_OK ? 0 : maxLen;
      }

      return 0;
    }

    inline void __attribute__((__always_inline__)) addHandler(UdpPacketHandlerFunction handler) noexcept {
      handlers.add(handler);
    }

    inline void __attribute__((__always_inline__)) removeHandler(UdpPacketHandlerFunction handler) noexcept {
      handlers.remove(handler);
    }

  private:
    void receive(AsyncUDPPacket& packet) noexcept;

    ArrayList<UdpPacketHandlerFunction, 64> handlers;
    uint16_t port;
    IPAddress ip;
    IPAddress localIp;

    bool started;
};

#endif