#include "logger.hpp"
#include "Udp.hpp"

static const char UDP_LOG[] PROGMEM = "UDP";

void Udp::receive(AsyncUDPPacket& packet) noexcept {
  // SP_LOGI(UDP_LOG, "Udp packet type: %s, remote ip: %s, local ip: %s, length=%d", 
  //               packet.isBroadcast() ? "Broadcast" : (packet.isMulticast() ? "Multicast" : "Unicast"),
  //               packet.remoteIP().toString().c_str(), packet.localIP().toString().c_str(), (int)packet.length());

  if ((uint32_t)packet.remoteIP() != localIp) {
    const UDP_MESSAGE *message = (const UDP_MESSAGE*)packet.data();
    const size_t length = packet.length();

    if (message->code > 0) {
      if (message->code == UDP_MESSAGE_ARRAY) {
        UDP_ARRAY_MESSAGE& array = (UDP_ARRAY_MESSAGE&)message;
        const uint8_t *ptr = array.data;
        for (uint16_t i = 0; i < array.count; i++) {
          const UDP_ARRAY_MESSAGE_ITEM &item = *(const UDP_ARRAY_MESSAGE_ITEM*)ptr;
          const UDP_MESSAGE &submessage = *(const UDP_MESSAGE*)item.data;
          const uint16_t len = item.length;
          
          if (submessage.code > 0 && submessage.code != UDP_MESSAGE_ARRAY)
            handlers.iterate([submessage, len] (UdpPacketHandlerFunction& func) { func(submessage, len); });

          ptr = item.data + item.length;
        }
      }
      else
        handlers.iterate([message, length] (UdpPacketHandlerFunction& func) { func(*message, length); });
    }
  }
}

bool Udp::begin(const IPAddress ip, uint16_t port, const IPAddress locIp, const char *hostname) noexcept {
  if (started) {
    if (this->ip == ip && this->port == port)
      return true;

    end();
  }

  localIp = locIp;
  this->ip = ip;
  this->port = port;

  if (ip[0] == 224) {
    // group
    if (listenMulticast(ip, port)) {
      onPacket([this] (AsyncUDPPacket packet) { receive(packet); });

      AsyncUDPMessage message;
      uint16_t code = UDP_HANDSHAKE_CODE;
      message.write((uint8_t*)&code, 2);
      message.write((uint8_t*)&localIp, 4);

      if (hostname)
        message.write((const uint8_t*)hostname, strlen(hostname));
      else
        message.write((const uint8_t*)"", 1);
        

      SP_LOGI(UDP_LOG, "Send handshake message: %d", (int)sendTo(message, ip, port));

      return started = true;
    }
    else
      SP_LOGE(UDP_LOG, "listenMulticast(): %s", ip.toString().c_str());
  }
  else {
    // if (listen(port)) {
    //   onPacket([this] (AsyncUDPPacket packet) { receive(packet); });
    // }
    // else
    //   SP_LOGE(UDP_LOG, "listen()");
    return started = true;
  }

  return started = false;
}

void Udp::end() noexcept {
  close();
  started = false;
}
