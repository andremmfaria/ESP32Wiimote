#include "l2cap_signaling.h"

#include "../../utils/serial_logging.h"
#include "../utils/hci_utils.h"
#include "../utils/payload_builder.h"

L2capSignaling::L2capSignaling() : connections(nullptr), sender(nullptr) {}

void L2capSignaling::init(L2capConnectionTable* connectionTable, L2capPacketSender* packetSender) {
    connections = connectionTable;
    sender = packetSender;
}

void L2capSignaling::sendConnectionRequest(uint16_t ch, uint16_t psm, uint16_t cid) {
    if (sender == nullptr) {
        LOG_ERROR("L2CAP: sendConnectionRequest failed: sender is null\n");
        return;
    }

    LOG_DEBUG("L2CAP: Sending connection request: ch=0x%04x psm=0x%04x cid=0x%04x\n", ch, psm, cid);

    PayloadBuilder pb(payload, sizeof(payload));
    pb.append(0x02);         // CONNECTION REQUEST
    pb.append(0x01);         // Identifier
    pb.appendU16LE(0x0004);  // Length
    pb.appendU16LE(psm);     // PSM
    pb.appendU16LE(cid);     // Source CID

    sender->sendAclL2capPacket(ch, 0x0001, payload, pb.length());
}

void L2capSignaling::handleConnectionResponse(uint16_t ch, uint8_t* data, uint16_t len) {
    if (len < 12) {
        LOG_DEBUG("L2CAP: Connection response too short: len=%d\n", len);
        return;
    }

    if (connections == nullptr || sender == nullptr) {
        LOG_ERROR("L2CAP: Connection response failed: null objects\n");
        return;
    }

    uint16_t dstCID = READ_UINT16_LE(data + 4);
    uint16_t result = READ_UINT16_LE(data + 8);

    LOG_DEBUG("L2CAP: Connection response: ch=0x%04x dstCID=0x%04x result=0x%04x\n", ch, dstCID,
              result);

    if (result != 0x0000) {
        LOG_WARN("L2CAP: Connection failed with result=0x%04x\n", result);
        return;
    }

    const L2capConnection connection(ch, dstCID);
    if (connections->addConnection(connection) < 0) {
        LOG_ERROR("L2CAP: Failed to add connection to table\n");
        return;
    }

    LOG_INFO("L2CAP: Connection established successfully\n");

    PayloadBuilder pb(payload, sizeof(payload));
    pb.append(0x04);         // CONFIGURATION REQUEST
    pb.append(0x02);         // Identifier
    pb.appendU16LE(0x0008);  // Length
    pb.appendU16LE(dstCID);  // Destination CID
    pb.appendU16LE(0x0000);  // Flags
    pb.append(0x01);         // Option: MTU
    pb.append(0x02);         // Option length
    pb.appendU16LE(0x0040);  // MTU value

    sender->sendAclL2capPacket(ch, 0x0001, payload, pb.length());
}

void L2capSignaling::handleConfigurationRequest(uint16_t ch, uint8_t* data, uint16_t len) {
    if (len < 12) {
        LOG_DEBUG("L2CAP: Config request too short: len=%d\n", len);
        return;
    }

    if (connections == nullptr || sender == nullptr) {
        LOG_ERROR("L2CAP: Config request failed: null objects\n");
        return;
    }

    uint8_t identifier = data[1];
    uint16_t dataLen = READ_UINT16_LE(data + 2);
    uint16_t flags = READ_UINT16_LE(data + 6);

    if (flags != 0x0000 || dataLen != 0x08 || data[8] != 0x01 || data[9] != 0x02) {
        return;
    }

    uint16_t mtu = READ_UINT16_LE(data + 10);
    uint16_t remoteCID = 0;
    if (connections->getRemoteCid(ch, &remoteCID) != 0) {
        LOG_DEBUG("L2CAP: Failed to get remote CID for ch=0x%04x\n", ch);
        return;
    }

    LOG_DEBUG("L2CAP: Config request: ch=0x%04x mtu=%d remoteCID=0x%04x\n", ch, mtu, remoteCID);

    PayloadBuilder pb(payload, sizeof(payload));
    pb.append(0x05);            // CONFIGURATION RESPONSE
    pb.append(identifier);      // Identifier
    pb.appendU16LE(0x000A);     // Length
    pb.appendU16LE(remoteCID);  // Source CID
    pb.appendU16LE(0x0000);     // Flags
    pb.appendU16LE(0x0000);     // Result: Success
    pb.append(0x01);            // Option: MTU
    pb.append(0x02);            // Option length
    pb.appendU16LE(mtu);        // MTU value

    sender->sendAclL2capPacket(ch, 0x0001, payload, pb.length());
}

void L2capSignaling::handleConfigurationResponse(uint8_t* data, uint16_t len) {
    (void)data;
    (void)len;
}
