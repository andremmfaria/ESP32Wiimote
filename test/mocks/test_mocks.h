// Native test mocks and lightweight implementations for protocol/parser tests.

#ifndef TEST_MOCKS_H
#define TEST_MOCKS_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../../src/esp32wiimote/data_parser.h"
#include "../../src/tinywiimote/l2cap/l2cap_connection.h"
#include "../../src/tinywiimote/l2cap/l2cap_packets.h"
#include "../../src/tinywiimote/protocol/wiimote_protocol.h"

// Shared mock state for TinyWiimote data parser tests.
static bool mockHasData = false;
static TinyWiimoteData mockData = {0, {0}, 0};

// Shared mock state for protocol packet tests.
static uint8_t mockLastPacket[256] = {0};
static int mockLastPacketLen = 0;
static int mockSendCallCount = 0;
static uint16_t mockLastChannelHandle = 0;
static uint16_t mockLastRemoteCID = 0;

inline void mockL2capRawSendCallback(uint8_t *data, size_t len) {
	mockSendCallCount++;

	if (len > sizeof(mockLastPacket)) {
		len = sizeof(mockLastPacket);
	}

	if (data != nullptr && len > 0) {
		memcpy(mockLastPacket, data, len);
	}
	mockLastPacketLen = (int)len;
}

// TinyWiimote input stubs used by WiimoteDataParser.
inline int TinyWiimoteAvailable(void) {
	return mockHasData ? 1 : 0;
}

inline TinyWiimoteData TinyWiimoteRead(void) {
	mockHasData = false;
	return mockData;
}

// Lightweight native implementations for classes used by tests.
inline L2capConnection::L2capConnection() : ch(0), remoteCID(0) {}

inline L2capConnection::L2capConnection(uint16_t channelHandle, uint16_t remoteCid)
	: ch(channelHandle), remoteCID(remoteCid) {}

inline L2capConnectionTable::L2capConnectionTable() : size(0) {}

inline void L2capConnectionTable::clear() {
	size = 0;
}

inline int L2capConnectionTable::findConnection(uint16_t ch) const {
	for (int i = 0; i < size; i++) {
		if (list[i].ch == ch) {
			return i;
		}
	}
	return -1;
}

inline int L2capConnectionTable::addConnection(const L2capConnection& connection) {
	if (size >= L2CAP_CONNECTION_LIST_SIZE) {
		return -1;
	}
	list[size++] = connection;
	return 0;
}

inline int L2capConnectionTable::getRemoteCid(uint16_t ch, uint16_t* remoteCID) const {
	const int idx = findConnection(ch);
	if (idx < 0 || remoteCID == nullptr) {
		return -1;
	}
	*remoteCID = list[idx].remoteCID;
	return 0;
}

inline int L2capConnectionTable::getFirstConnectionHandle(uint16_t* ch) const {
	if (size == 0 || ch == nullptr) {
		return -1;
	}
	*ch = list[0].ch;
	return 0;
}

inline L2capPacketSender::L2capPacketSender() : sendCallback(nullptr), tmpQueueData{0} {}

inline void L2capPacketSender::setSendCallback(L2capRawSendFunc callback) {
	sendCallback = callback;
}

inline void L2capPacketSender::sendAclL2capPacket(uint16_t ch, uint16_t remoteCID, uint8_t* payload,
												   uint16_t payloadLen) {
	mockLastChannelHandle = ch;
	mockLastRemoteCID = remoteCID;

	if (sendCallback == nullptr || payload == nullptr) {
		return;
	}

	// Native tests verify the Wiimote output payload directly.
	sendCallback(payload, payloadLen);
}

inline uint16_t make_l2cap_packet(uint8_t* buf, uint16_t channelID, uint8_t* data, uint16_t len) {
	(void)buf;
	(void)channelID;
	(void)data;
	return len;
}

inline uint16_t make_acl_l2cap_packet(uint8_t* buf, uint16_t ch, uint8_t pbf, uint8_t bf,
									  uint16_t channelID, uint8_t* data, uint8_t len) {
	(void)ch;
	(void)pbf;
	(void)bf;
	(void)channelID;
	if (buf != nullptr && data != nullptr && len > 0) {
		memcpy(buf, data, len);
	}
	return len;
}

inline WiimoteProtocol::WiimoteProtocol() : connections(nullptr), sender(nullptr), payload{0} {}

inline void WiimoteProtocol::init(const L2capConnectionTable* connectionTable,
								  L2capPacketSender* packetSender) {
	connections = connectionTable;
	sender = packetSender;
}

inline bool WiimoteProtocol::isValidMemorySize(uint16_t size, const char* operation) const {
	(void)operation;
	return size <= 16;
}

inline void WiimoteProtocol::setLeds(uint16_t ch, uint8_t leds) {
	if (connections == nullptr || sender == nullptr) {
		return;
	}

	uint16_t remoteCID = 0;
	if (connections->getRemoteCid(ch, &remoteCID) != 0) {
		return;
	}

	payload[0] = 0xA2;
	payload[1] = 0x11;
	payload[2] = (uint8_t)(leds << 4);
	sender->sendAclL2capPacket(ch, remoteCID, payload, 3);
}

inline void WiimoteProtocol::setReportingMode(uint16_t ch, uint8_t mode, bool continuous) {
	if (connections == nullptr || sender == nullptr) {
		return;
	}

	uint16_t remoteCID = 0;
	if (connections->getRemoteCid(ch, &remoteCID) != 0) {
		return;
	}

	payload[0] = 0xA2;
	payload[1] = 0x12;
	payload[2] = continuous ? 0x04 : 0x00;
	payload[3] = mode;
	sender->sendAclL2capPacket(ch, remoteCID, payload, 4);
}

inline void WiimoteProtocol::requestStatus(uint16_t ch) {
	if (connections == nullptr || sender == nullptr) {
		return;
	}

	uint16_t remoteCID = 0;
	if (connections->getRemoteCid(ch, &remoteCID) != 0) {
		return;
	}

	payload[0] = 0xA2;
	payload[1] = 0x15;
	payload[2] = 0x00;
	sender->sendAclL2capPacket(ch, remoteCID, payload, 3);
}

inline void WiimoteProtocol::writeMemory(uint16_t ch, address_space_t address_space,
										 uint32_t offset, const uint8_t* data,
										 uint8_t length) {
	if (connections == nullptr || sender == nullptr || data == nullptr) {
		return;
	}
	if (!isValidMemorySize(length, "Write")) {
		return;
	}

	uint16_t remoteCID = 0;
	if (connections->getRemoteCid(ch, &remoteCID) != 0) {
		return;
	}

	const uint8_t addressSpaceByte = (address_space == CONTROL_REGISTER) ? 0x04 : 0x00;
	payload[0] = 0xA2;
	payload[1] = 0x16;
	payload[2] = addressSpaceByte;
	payload[3] = (uint8_t)((offset >> 16) & 0xFF);
	payload[4] = (uint8_t)((offset >> 8) & 0xFF);
	payload[5] = (uint8_t)(offset & 0xFF);
	payload[6] = length;

	memset(&payload[7], 0, 16);
	if (length > 0) {
		memcpy(&payload[7], data, length);
	}

	sender->sendAclL2capPacket(ch, remoteCID, payload, 23);
}

inline void WiimoteProtocol::readMemory(uint16_t ch, address_space_t address_space,
										uint32_t offset, uint16_t size) {
	if (connections == nullptr || sender == nullptr) {
		return;
	}
	if (!isValidMemorySize(size, "Read")) {
		return;
	}

	uint16_t remoteCID = 0;
	if (connections->getRemoteCid(ch, &remoteCID) != 0) {
		return;
	}

	const uint8_t addressSpaceByte = (address_space == CONTROL_REGISTER) ? 0x04 : 0x00;
	payload[0] = 0xA2;
	payload[1] = 0x17;
	payload[2] = addressSpaceByte;
	payload[3] = (uint8_t)((offset >> 16) & 0xFF);
	payload[4] = (uint8_t)((offset >> 8) & 0xFF);
	payload[5] = (uint8_t)(offset & 0xFF);
	payload[6] = (uint8_t)((size >> 8) & 0xFF);
	payload[7] = (uint8_t)(size & 0xFF);

	sender->sendAclL2capPacket(ch, remoteCID, payload, 8);
}

inline WiimoteDataParser::WiimoteDataParser(ButtonStateManager* buttonState,
											SensorStateManager* sensorState)
	: _buttonState(buttonState), _sensorState(sensorState), _filter(FILTER_NONE) {}

inline int WiimoteDataParser::parseData(void) {
	int buttonChanged = false;
	int accelChanged = false;
	int nunchukStickChanged = false;

	if (!TinyWiimoteAvailable()) {
		return 0;
	}

	TinyWiimoteData rd = TinyWiimoteRead();
	if (rd.len < 4) {
		return 0;
	}
	if (rd.data[0] != 0xA1) {
		return 0;
	}

	_buttonState->resetChangeFlag();
	_sensorState->resetChangeFlags();

	parseButtonData(rd, buttonChanged);
	parseAccelData(rd, accelChanged);
	parseNunchukData(rd, nunchukStickChanged, accelChanged, buttonChanged);

	return (buttonChanged | nunchukStickChanged | accelChanged);
}

inline void WiimoteDataParser::setFilter(int filter) {
	_filter = filter;
}

inline int WiimoteDataParser::getFilter(void) const {
	return _filter;
}

inline void WiimoteDataParser::parseButtonData(const TinyWiimoteData& data, int& buttonChanged) {
	int offs = 0;

	if ((data.data[1] >= 0x30) && (data.data[1] <= 0x37)) {
		offs = 2;
		const uint16_t rawButtons = (data.data[1] == 0x31)
			? (uint16_t)((data.data[offs] << 8) | data.data[offs + 1])
			: (uint16_t)(data.data[offs] | data.data[offs + 1]);
		ButtonState buttonState = (ButtonState)rawButtons;
		_buttonState->update(buttonState);
	}

	if (!(_filter & FILTER_BUTTON) && _buttonState->hasChanged()) {
		buttonChanged = true;
	}
}

inline void WiimoteDataParser::parseAccelData(const TinyWiimoteData& data, int& accelChanged) {
	int offs = 0;

	switch (data.data[1]) {
		case 0x31:
		case 0x35:
			offs = 4;
			break;
		default:
			offs = 0;
			break;
	}

	if (offs) {
		AccelState accelState;
		accelState.xAxis = data.data[offs + 0];
		accelState.yAxis = data.data[offs + 1];
		accelState.zAxis = data.data[offs + 2];
		_sensorState->updateAccel(accelState);

		if (!(_filter & FILTER_ACCEL)) {
			accelChanged = true;
		}
	} else {
		_sensorState->resetAccel();
	}
}

inline void WiimoteDataParser::parseNunchukData(const TinyWiimoteData& data,
												int& nunchukStickChanged,
												int& accelChanged,
												int& buttonChanged) {
	(void)accelChanged;
	(void)buttonChanged;
	int offs = 0;

	switch (data.data[1]) {
		case 0x32:
			offs = 4;
			break;
		case 0x35:
			offs = 7;
			break;
		default:
			offs = 0;
			break;
	}

	if (offs) {
		NunchukState nunchukState;
		nunchukState.xStick = data.data[offs + 0];
		nunchukState.yStick = data.data[offs + 1];
		nunchukState.xAxis = data.data[offs + 2];
		nunchukState.yAxis = data.data[offs + 3];
		nunchukState.zAxis = data.data[offs + 4];

		_sensorState->updateNunchuk(nunchukState);

		const uint8_t cBtn = ((data.data[offs + 5] & 0x02) >> 1) ^ 0x01;
		const uint8_t zBtn = (data.data[offs + 5] & 0x01) ^ 0x01;

		ButtonState buttonState = _buttonState->getCurrent();
		if (cBtn) {
			buttonState = (ButtonState)((int)buttonState | BUTTON_C);
		}
		if (zBtn) {
			buttonState = (ButtonState)((int)buttonState | BUTTON_Z);
		}
		_buttonState->update(buttonState);

		if (!(_filter & FILTER_NUNCHUK_STICK) && _sensorState->nunchukStickHasChanged()) {
			nunchukStickChanged = true;
		}
	} else {
		_sensorState->resetNunchuk();
	}
}

#endif // TEST_MOCKS_H
