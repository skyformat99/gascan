
#ifndef _PACKET_H_
#define _PACKET_H_

#define PACKET_OK              0
#define PACKET_NOT_COMPLETE    1
#define PACKET_INVALID         2

extern uint8 BuildMeasureDataPacket(uint8 *buf, uint8 maxBufLen, uint8 result, uint8 reason, uint8 dataMap,
												const uint16 *data);

extern uint8 BuildTestMeasureDataPacket(uint8 *buf, uint8 maxBufLen, uint8 result, uint8 reason, uint8 dataMap,
												const uint16 *data);
												
extern uint8 BuildStartCalibrationRetPacket(uint8 *buf, uint8 maxBufLen, uint8 type, uint8 start, uint8 result, uint8 reason);

extern uint8 BuildTemperatureCalibrateRetPacket(uint8 *buf, uint8 maxBufLen, uint16 temperature, uint8 result, uint8 reason);

extern uint8 BuildPressureCalibrateRetPacket(uint8 *buf, uint8 maxBufLen, uint16 kPa, uint8 result, uint8 reason);

extern uint8 BuildSetBleNameRetPacket(uint8 *buf, uint8 maxBufLen, uint8 result);

extern uint8 ParsePacket(const uint8 *buf, uint8 len, uint8 *parsedLen);

extern void ResetParsePacket();

extern const uint8 *GetPacketData();

extern uint8 GetPacketType();
extern uint8 GetPacketDataLen();

extern bool ParseGetMesureDataPacket(const uint8 * data, uint8 len, uint8 *dataMap);

extern bool ParseStartCalibrationPacket(const uint8 *data, uint8 len, uint8 *type, uint8 *start);

extern bool ParseTemperatureCalibratePacket(const uint8 *data, uint8 len, uint16 *temperature);

extern bool ParsePressureCalibratePacket(const uint8 *data, uint8 len, uint16 *kPa);

extern bool ParseSetBleNamePacket(const uint8 *data, uint8 len, uint8 name[BLE_NAME_LEN]);

extern bool ParseTestPacket(const uint8 *data, uint8 len, uint8 *dataMap, uint16 *testData);

#endif

