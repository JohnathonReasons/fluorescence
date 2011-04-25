#ifndef UOME_NET_PACKETS_UNKNOWN_HPP
#define UOME_NET_PACKETS_UNKNOWN_HPP

#include <net/packet.hpp>

namespace uome {
namespace net {

namespace packets {

class Unknown : public Packet {
public:
    Unknown(uint8_t id);

    virtual bool read(const int8_t* buf, unsigned int len, unsigned int& index);

private:

};

}
}
}

#endif
