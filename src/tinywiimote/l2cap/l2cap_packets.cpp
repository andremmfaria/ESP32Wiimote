// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "l2cap_packets.h"

#include "../../utils/serial_logging.h"
#include "../utils/hci_utils.h"

L2capPacketSender::L2capPacketSender() = default;

void L2capPacketSender::setSendCallback(L2capRawSendFunc callback) {
    sendCallback_ = callback;
}

uint16_t makeL2capPacket(uint8_t *buf, uint16_t channelID, const uint8_t *data, uint16_t len) {
    streamU16ToLe(buf, len);
    streamU16ToLe(buf, channelID);
    streamArray(buf, data, len);
    return L2CAP_HEADER_LEN + len;
}

uint16_t makeAclL2capPacket(uint8_t *buf,
                            uint16_t ch,
                            const AclPacketControl &control,
                            uint16_t channelID,
                            uint8_t *data,
                            uint8_t len) {
    uint8_t *l2capBuf = buf + HCI_H4_ACL_PREAMBLE_SIZE;
    uint16_t l2capLen = makeL2capPacket(l2capBuf, channelID, data, len);

    streamU8ToLe(buf, static_cast<uint8_t>(H4PacketType::Acl));
    streamU8ToLe(buf, ch & 0xFF);
    streamU8ToLe(
        buf, ((ch >> 8) & 0x0F) | (control.packetBoundaryFlag << 4) | (control.broadcastFlag << 6));
    streamU16ToLe(buf, l2capLen);

    return HCI_H4_ACL_PREAMBLE_SIZE + l2capLen;
}

void L2capPacketSender::sendAclL2capPacket(uint16_t ch,
                                           uint16_t remoteCID,
                                           uint8_t *payload,
                                           uint16_t payloadLen) {
    if (sendCallback_ == nullptr) {
        LOG_ERROR("L2CAP: sendCallback is null\n");
        return;
    }

    if (payloadLen > 255) {
        LOG_ERROR("L2CAP: Payload too large: %d bytes (max 255)\n", payloadLen);
        return;
    }

    LOG_DEBUG("L2CAP: Sending ACL packet: ch=0x%04x remoteCID=0x%04x len=%d\n", ch, remoteCID,
              payloadLen);

    const AclPacketControl kControl = {0b10, 0b00};

    const uint16_t kPacketLen =
        makeAclL2capPacket(tmpQueueData_, ch, kControl, remoteCID, payload, (uint8_t)payloadLen);
    sendCallback_(tmpQueueData_, kPacketLen);
}
