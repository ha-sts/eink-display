#include "mqtt_client.h"

#define MQTT_PROTOCOL_LEVEL 0x04 // Hardcoded to 3.1.1 for now
#define MQTT_SUBSCRIBE_REQUEST_MAX_NUM_TOPICS 8
#define MQTT_UNSUBSCRIBE_REQUEST_MAX_NUM_TOPICS 8

// http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718021
enum class MQTTControlPacketType : uint8_t {
    Forbidden0 = 0, // Reserved - Forbidden
    Connect = 1, // Client request to connect to Server
    ConnAck = 2, // Connect acknowledgment
    Publish = 3, // Publish message
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

// Need to add the variable string class
// - default constructor
// - string input constructor
class MQTTString {
    public:
        std::string contents;

        MQTTString() {}

        MQTTString(std::string in) {
            contents = in;
        }

        std::vector<uint8_t> getBytes() {
            std::vector<uint8_t> result;
            if(contents.size() < 65536) {
                result.push_back(contents.size() / 256);
                result.push_back(contents.size() % 256);
                for(char& c : contents) {
                    result.push_back(c);
                }
            }
            return result;
        }
};

class MQTTControlPacketVariableHeader {
    public:
        virtual std::vector<uint8_t> getBytes();
};

class MQTTControlPacketVariableHeader_Connect : public MQTTControlPacketVariableHeader {
    public:
        std::vector<uint8_t> protocolName = MQTTString("MQTT").getBytes();
        uint16_t keepaliveInterval = 0;
        uint8_t protocolLevel = MQTT_PROTOCOL_LEVEL;
        MQTTQoSType qos = MQTTQoSType::AtMostOnce;
        bool usernameFlag = false; // 0b10000000
        bool passwordFlag = false; // 0b01000000
        bool retainFlag = false; // 0b00100000
        bool willFlag = false; // 0b00000100
        bool cleanSessionFlag = false; // 0b00000010

        std::vector<uint8_t> getBytes() override {
            std::vector<uint8_t> result;
            result.insert(result.end(), protocolName.begin(), protocolName.end());
            result.push_back(protocolLevel);
            result.push_back((usernameFlag << 7) & (passwordFlag << 6) & (retainFlag << 5) & (static_cast<uint8_t>(qos) << 3) & (willFlag << 2) & (cleanSessionFlag << 1));
            return result;
        }
};

// FIXME:  Add the rest of the variable headers for the command types.

class MQTTControlPacketVariableHeader_PIV {
    public:
        uint16_t packetIdentifierValue;

        MQTTControlPacketVariableHeader_PIV() {
            // Load the PIV with a random value
            packetIdentifierValue = (uint16_t)(rand() % 65536);
        }

        MQTTControlPacketVariableHeader_PIV(std::vector<uint8_t> bytes){
            // The vector should be two bytes long.
            packetIdentifierValue = bytes[0] * 256 + bytes[1];
        }

        std::vector<uint8_t> getBytes() {
            std::vector<uint8_t> result;
            result.push_back(packetIdentifierValue / 256); // Insert MSB
            result.push_back(packetIdentifierValue % 256); // Insert LSB
            return result;
        }
};

class MQTTControlPacketPayload {
    public:
        MQTTControlPacketPayload() {}
};

class MQTTControlPacket {
    public:
        MQTTControlPacketType type = MQTTControlPacketType::PingReq;
        MQTTQoSType qos = MQTTQoSType::AtMostOnce;
        bool dup = false; // false: First attempt to send packet, true: Might be a duplicate packet
        bool retain = false;

        uint32_t remainingLength = 0;

        // variable header
        MQTTControlPacketVariableHeader_PIV variableHeader;

        // payload

        bool packetError = false;

        MQTTControlPacket() {}

        MQTTControlPacket(std::vector<uint8_t> in_packet) {
            if(in_packet.size()) {
                type = static_cast<MQTTControlPacketType>((in_packet[0] & 0b11110000) >> 4);
                qos = static_cast<MQTTQoSType>((in_packet[0] & 0b00000110) >> 1);
                dup = (in_packet[0] & 0b00001000) > 0;
                retain = (in_packet[0] & 0b00000001) > 0;

                uint8_t remainingLengthBytes = decodeRemainingLength(in_packet);

                uint8_t variableHeaderBytes = decodeVariableHeader(in_packet, remainingLengthBytes + 1);

                // Payload
            }
        }

    private:
        uint8_t getHeaderByte() {
            return (static_cast<uint8_t>(type) << 4) | (dup << 3) | (static_cast<uint8_t>(qos) << 1) | (retain << 0);
        }

        uint8_t decodeRemainingLength(std::vector<uint8_t> tmp_packet) {
            uint32_t multiplier = 1;
            uint32_t value = 0;
            uint8_t index = 1; // Start at the second byte in the packet
            do {
                value += (tmp_packet[index] & 0b01111111) * multiplier;
                multiplier *= 128;
                if(index > 4) {
                    packetError = true;
                    return 0; // Should really throw an exception here.
                }
                index++;
            } while((tmp_packet[index] & 0b10000000) != 0);
            remainingLength = value;
            return index - 1; // The number of bytes in the "remainingLength" field.
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

        uint8_t decodeVariableHeader(std::vector<uint8_t> tmp_packet, uint8_t start_index) {
            uint8_t decodedByteCount = 0;
            std::vector<uint8_t> tmp_vh;
            switch(type) {
                case MQTTControlPacketType::PubAck:
                case MQTTControlPacketType::PubRec:
                case MQTTControlPacketType::PubRel:
                case MQTTControlPacketType::PubComp:
                case MQTTControlPacketType::Subscribe:
                case MQTTControlPacketType::SubAck:
                case MQTTControlPacketType::Unsubscribe:
                case MQTTControlPacketType::UnsubAck:
                    // Create a variable header setting PIV
                    tmp_vh.push_back(tmp_packet[start_index]);
                    tmp_vh.push_back(tmp_packet[start_index+1]);
                    variableHeader = MQTTControlPacketVariableHeader_PIV(tmp_vh);
                    decodedByteCount = tmp_vh.size();
                    break;
                case MQTTControlPacketType::Publish:
                    // Create a variable header setting PIV if QOS > 0
                    if(qos != MQTTQoSType::AtMostOnce) {
                        tmp_vh.push_back(tmp_packet[start_index]);
                        tmp_vh.push_back(tmp_packet[start_index+1]);
                        variableHeader = MQTTControlPacketVariableHeader_PIV(tmp_vh);
                        decodedByteCount = tmp_vh.size();
                    }
                    // Otherwise create a default variable header (it won't be used)
                    else {
                        variableHeader = MQTTControlPacketVariableHeader_PIV();
                    }
                    break;
                default:
                    // Create a default variable header (it won't be used)
                    variableHeader = MQTTControlPacketVariableHeader_PIV();
                    break;
            }
            return decodedByteCount;
        }

        std::vector<uint8_t> encodeVariableHeaderBytes() {
            std::vector<uint8_t> result;
            switch(type) {
                case MQTTControlPacketType::PubAck:
                case MQTTControlPacketType::PubRec:
                case MQTTControlPacketType::PubRel:
                case MQTTControlPacketType::PubComp:
                case MQTTControlPacketType::Subscribe:
                case MQTTControlPacketType::SubAck:
                case MQTTControlPacketType::Unsubscribe:
                case MQTTControlPacketType::UnsubAck:
                    // Return a PIV variable header
                    result = variableHeader.getBytes();
                    break;
                case MQTTControlPacketType::Publish:
                    // Return a PIV variable header if QOS > 0
                    if(qos != MQTTQoSType::AtMostOnce) {
                        result = variableHeader.getBytes();
                    }
                    break;
                default:
                    break;
            }
            return result;
        }
};

mqtt_client::mqtt_client() {
    srand(time(0));
}
