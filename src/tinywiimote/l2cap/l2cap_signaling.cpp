#include "l2cap_signaling.h"

#include "../../utils/protocol_codes.h"
#include "../../utils/serial_logging.h"
#include "../utils/hci_utils.h"
#include "../utils/payload_builder.h"

L2capSignaling::L2capSignaling() = default;

void L2capSignaling::init(L2capConnectionTable *connectionTable, L2capPacketSender *packetSender) {
    connections_ = connectionTable;
    sender_ = packetSender;
}

void L2capSignaling::sendConnectionRequest(uint16_t ch, uint16_t psm, uint16_t cid) {
    if (sender_ == nullptr) {
        LOG_ERROR("L2CAP: sendConnectionRequest failed: sender is null\n");
        return;
    }

    LOG_DEBUG("L2CAP: Sending %s on CID=0x%04x: ch=0x%04x psm=0x%04x cid=0x%04x\n",
              l2capSignalCodeToString((uint8_t)L2capSignalingCode::ConnectionRequest),
              (uint16_t)L2capCid::SIGNALING, ch, psm, cid);

    PayloadBuilder pb(payload_, sizeof(payload_));
    pb.append((uint8_t)L2capSignalingCode::ConnectionRequest);
    pb.append(0x01);         // Identifier
    pb.appendU16LE(0x0004);  // Length
    pb.appendU16LE(psm);     // PSM
    pb.appendU16LE(cid);     // Source CID

    sender_->sendAclL2capPacket(ch, (uint16_t)L2capCid::SIGNALING, payload_, pb.length());
}

void L2capSignaling::handleConnectionResponse(uint16_t ch, uint8_t *data, uint16_t len) {
    if (len < 12) {
        LOG_DEBUG("L2CAP: Connection response too short: len=%d\n", len);
        return;
    }

    if (connections_ == nullptr || sender_ == nullptr) {
        LOG_ERROR("L2CAP: Connection response failed: null objects\n");
        return;
    }

    uint16_t dstCID = readUInt16Le(data + 4);
    uint16_t result = readUInt16Le(data + 8);

    LOG_DEBUG("L2CAP: %s: ch=0x%04x dstCID=0x%04x result=0x%04x (%s)\n",
              l2capSignalCodeToString((uint8_t)L2capSignalingCode::ConnectionResponse), ch, dstCID,
              result, l2capSignalingResultToString(result));

    if (result != (uint16_t)L2capSignalingResult::SUCCESS) {
        LOG_WARN("L2CAP: Connection failed with result=0x%04x (%s)\n", result,
                 l2capSignalingResultToString(result));
        return;
    }

    L2capConnection::Endpoint endpoint = {ch, dstCID};
    const L2capConnection kConnection(endpoint);
    if (connections_->addConnection(kConnection) < 0) {
        LOG_ERROR("L2CAP: Failed to add connection to table\n");
        return;
    }

    LOG_INFO("L2CAP: Connection established successfully\n");

    PayloadBuilder pb(payload_, sizeof(payload_));
    pb.append((uint8_t)L2capSignalingCode::ConfigurationRequest);
    pb.append(0x02);         // Identifier
    pb.appendU16LE(0x0008);  // Length
    pb.appendU16LE(dstCID);  // Destination CID
    pb.appendU16LE(0x0000);  // Flags
    pb.append(0x01);         // Option: MTU
    pb.append(0x02);         // Option length
    pb.appendU16LE(0x0040);  // MTU value

    sender_->sendAclL2capPacket(ch, (uint16_t)L2capCid::SIGNALING, payload_, pb.length());
}

void L2capSignaling::handleConfigurationRequest(uint16_t ch, uint8_t *data, uint16_t len) {
    if (len < 12) {
        LOG_DEBUG("L2CAP: Config request too short: len=%d\n", len);
        return;
    }

    if (connections_ == nullptr || sender_ == nullptr) {
        LOG_ERROR("L2CAP: Config request failed: null objects\n");
        return;
    }

    uint8_t identifier = data[1];
    uint16_t dataLen = readUInt16Le(data + 2);
    uint16_t flags = readUInt16Le(data + 6);

    if (flags != 0x0000 || dataLen != 0x08 || data[8] != 0x01 || data[9] != 0x02) {
        return;
    }

    uint16_t mtu = readUInt16Le(data + 10);
    uint16_t remoteCID = 0;
    if (connections_->getRemoteCid(ch, &remoteCID) != 0) {
        LOG_DEBUG("L2CAP: Failed to get remote CID for ch=0x%04x\n", ch);
        return;
    }

    LOG_DEBUG("L2CAP: %s: ch=0x%04x mtu=%d remoteCID=0x%04x\n",
              l2capSignalCodeToString((uint8_t)L2capSignalingCode::ConfigurationRequest), ch, mtu,
              remoteCID);

    PayloadBuilder pb(payload_, sizeof(payload_));
    pb.append((uint8_t)L2capSignalingCode::ConfigurationResponse);
    pb.append(identifier);      // Identifier
    pb.appendU16LE(0x000A);     // Length
    pb.appendU16LE(remoteCID);  // Source CID
    pb.appendU16LE(0x0000);     // Flags
    pb.appendU16LE(0x0000);     // Result: Success
    pb.append(0x01);            // Option: MTU
    pb.append(0x02);            // Option length
    pb.appendU16LE(mtu);        // MTU value

    sender_->sendAclL2capPacket(ch, (uint16_t)L2capCid::SIGNALING, payload_, pb.length());
}

void L2capSignaling::handleConfigurationResponse(const uint8_t *data, uint16_t len) {
    (void)data;
    (void)len;
}
