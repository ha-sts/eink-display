#include "mqtt_client.h"

#define MQTT_PROTOCOL_LEVEL 0x04
#define MQTT_SUBSCRIBE_REQUEST_MAX_NUM_TOPICS 8
#define MQTT_UNSUBSCRIBE_REQUEST_MAX_NUM_TOPICS 8

// http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718021
enum class MQTTControlPacketType : uint8_t {
    Forbidden0 = 0, // Reserved - Forbidden
    Connect = 1, // Client request to connect to Server
    ConnAck = 2, // Connect acknowledgment
    Publich = 3, // Publish message
    PubAck = 4, // Publish acknowledgment
    PubRec = 5, // Publish received (assured delivery part 1)
    PubRel = 6, // Publish release (assured delivery part 2)
    PubComp = 7, // Publish complete (assured delivery part 3)
    Subscribe = 8, // Client subscribe request
    SubAck = 9, // Subscribe acknowledgment
    Unsubscribe = 10, // Unsubscribe request
    UnsubAck = 11, // Unsubscribe acknowledgment
    PingReq = 12, // Ping request
    PingResp = 13, // Ping response
    Disconnect = 14, // Client is disconnecting
    Forbidden15 = 15 // Reserved - Forbidden
};

enum class MQTTQoSType : uint8_t {
    AtMostOnce = 0, // Only attempt to deliver the packet once (a.k.a. send and forget)
    AtLeastOnce = 1, // Attempt to deliver the packet, retrying if necessary.  Multiples may be delivered.
    ExactlyOnce = 2, // Enable some fancy stuff to ensure the packet is delivered and processed only once.
    Forbidden = 3 // Reserved - Forbidden
};

enum class MQTTConnAckReturnCode : uint8_t {
    Accepted = 0,
    ProtocolVersion = 1,
    IdentifierRejected = 2,
    ServerUnavailable = 3,
    BadUsernameOrPassword = 4,
    NotAuthorized = 5
};

enum class MQTTSubAckReturnCode : uint8_t {
    SuccessQOS0 = 0,
    SuccessQOS1 = 1,
    SuccessQOS2 = 2,
    Failure = 128
};

enum class MQTTConnectFlags : uint8_t {
    Reserved = 0b00000001,
    CleanSession = 0b00000010,
    Will = 0b00000100,
    QOS1 = 0b00001000,
    QOS2 = 0b00010000,
    Retain = 0b00100000,
    Password = 0b01000000,
    Username = 0b10000000
};

class MQTTControlPacket {
    public:
        MQTTControlPacketType type;
        MQTTQoSType qos;
        bool dup; // false: First attempt to send packet, true: Might be a duplicate packet
        bool retain;

        uint32_t remainingLength;

        // variable header?

        // payload

        bool packetError;

        MQTTControlPacket() {
            type = MQTTControlPacketType::PingReq;
            dup = false;
            qos = MQTTQoSType::AtMostOnce;
            retain = false;

            remainingLength = 0;

            packetError = false;
        }

        /* MQTTControlPacket(...) {
            ...
            type = static_cast<MQTTControlPacketType>((header & 0b11110000) >> 4);
            qos = static_cast<MQTTQoSType>((header & 0b00000110) >> 1);
            dup = (header & 0b00001000) > 0;
            retain = (header & 0b00000001) > 0;
            ...
        } */

    private:
        void decodeRemainingLength(std::vector<uint8_t> tmp_packet) {
            uint32_t multiplier = 1;
            uint32_t value = 0;
            uint8_t index = 1; // Start at the second byte in the packet
            do {
                value += (tmp_packet[index] & 0b01111111) * multiplier;
                multiplier *= 128;
                if(index > 4) {
                    packetError = true;
                    return;
                }
                index++;
            } while((tmp_packet[index] & 0b10000000) != 0);
            remainingLength = value;
        }

        std::vector<uint8_t> encodeRemainingLength() {
            std::vector<uint8_t> encoded;
            uint32_t tmp_remain = remainingLength;
            do {
                uint8_t encodedByte = tmp_remain % 128;
                tmp_remain /= 128;
                if(tmp_remain > 0) {
                    encodedByte |= 128;
                }
                encoded.push_back(encodedByte);
            } while(tmp_remain > 0);
            return encoded;
        }

        /*
        uint8_t getHeaderByte() {
            return (static_cast<uint8_t>(type) << 4) | (dup << 3) | (static_cast<uint8_t>(qos) << 1) | (retain << 0);
        }

        bool getHeaderError() {
            if(type == MQTTControlPacketType::Forbidden0) return true;
            if(type == MQTTControlPacketType::Forbidden15) return true;
            if(qos == MQTTQoSType::Forbidden) return true;
            // Add more validation checks here.  This should be converted to exception handling, but that's for later...
            return false;
        }
        */
};

mqtt_client::mqtt_client() {
    //ctor
}
