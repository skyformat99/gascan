#include "hal_types.h"
#include "comdef.h"

#include "Protocol.h"

#include "Packet.h"

#include "npi.h"

static uint8 s_recvBuf[MAX_PACKET_LEN];

typedef enum
{
	recv_status_idle = 0,
	recv_status_version,
	recv_status_length,
	recv_status_data,
	recv_status_sum,
} BleReceiveStatus;

static BleReceiveStatus s_recvStatus = recv_status_idle;

static uint8 s_dataRecvIndex = 0;

static uint8 BuildPacket(uint8 *buf, uint8 maxBufLen, const uint8 *data, uint8 len)
{
	if (1 + 1 + 2 + len + 1 > maxBufLen)
	{
		return 0xff;
	}

	uint8 index = 0;

	buf[index++] = PACKET_HEADER;

	buf[index++] = PROTOCOL_VERSION;
	
	buf[index++] = (len + 1) >> 8;
	buf[index++] = (len + 1);

	uint8 sum = 0;
	for (uint8 i = 0; i < len; i++)
	{
		buf[index++] = *data;

		sum ^= *data;

		data++;
	}

	buf[index++] = sum;

	return index;
}

static uint8 BuildDataPacket(uint8 *buf, uint8 maxBufLen, uint8 result, uint8 reason, uint8 type, uint8 dataMap,
												const uint16 *data)
{
	uint8 dataBuf[MAX_PACKET_LEN];
	
	uint8 index = 0;
	dataBuf[index++] = type;
	
	dataBuf[index++] = result;
	dataBuf[index++] = reason;

	dataBuf[index++] = dataMap;
	
	if (dataMap & DATA_TEMPERATURE_MASK)
	{
		dataBuf[index++] = *data >> 8;
		dataBuf[index++] = *data & 0xff;
	
		data++;
	}

	if (dataMap & DATA_PRESSURE_MASK)
	{
		dataBuf[index++] = *data >> 8;
		dataBuf[index++] = *data & 0xff;

		data++;
	}

	if (dataMap & DATA_VOLTAGE_MASK)
	{
		dataBuf[index++] = *data >> 8;
		dataBuf[index++] = *data & 0xff;
		
		data++;
	}
	
	return BuildPacket(buf, maxBufLen, dataBuf, index);
}

uint8 BuildMeasureDataPacket(uint8 *buf, uint8 maxBufLen, uint8 result, uint8 reason, uint8 dataMap,
												const uint16 *data)
{
	return BuildDataPacket(buf, maxBufLen, result, reason, TYPE_MEASURE, dataMap, data);
}

uint8 BuildTestMeasureDataPacket(uint8 *buf, uint8 maxBufLen, uint8 result, uint8 reason, uint8 dataMap,
												const uint16 *data)
{
	return BuildDataPacket(buf, maxBufLen, result, reason, TYPE_TEST, dataMap, data);
}

uint8 BuildStartCalibrationRetPacket(uint8 *buf, uint8 maxBufLen, uint8 type, uint8 start, uint8 reault, uint8 reason)
{
	uint8 dataBuf[MAX_PACKET_LEN];
		
	uint8 index = 0;
	dataBuf[index++] = TYPE_START_CALIBRATION;

	dataBuf[index++] = type;
	dataBuf[index++] = start;
	dataBuf[index++] = reault;
	dataBuf[index++] = reason;
	
	return BuildPacket(buf, maxBufLen, dataBuf, index);
}

uint8 BuildTemperatureCalibrateRetPacket(uint8 *buf, uint8 maxBufLen, uint16 temperature, uint8 result, uint8 reason)
{
	uint8 dataBuf[MAX_PACKET_LEN];
		
	uint8 index = 0;
	dataBuf[index++] = TYPE_TEMP_CALIBRATE;

	dataBuf[index++] = temperature >> 8;
	dataBuf[index++] = temperature & 0xff;

	dataBuf[index++] = result;
	dataBuf[index++] = reason;
	
	return BuildPacket(buf, maxBufLen, dataBuf, index);
}

uint8 BuildPressureCalibrateRetPacket(uint8 *buf, uint8 maxBufLen, uint16 kPa, uint8 result, uint8 reason)
{
	uint8 dataBuf[MAX_PACKET_LEN];
	
	uint8 index = 0;
	dataBuf[index++] = TYPE_PRESSURE_CALIBRATE;
	
	dataBuf[index++] = kPa >> 8;
	dataBuf[index++] = kPa & 0xff;

	dataBuf[index++] = result;
	dataBuf[index++] = reason;
	
	return BuildPacket(buf, maxBufLen, dataBuf, index);

}

uint8 BuildSetBleNameRetPacket(uint8 *buf, uint8 maxBufLen, uint8 result)
{
	uint8 dataBuf[MAX_PACKET_LEN];
	
	uint8 index = 0;
	dataBuf[index++] = TYPE_SET_BLE_NAME;

	dataBuf[index++] = result;
	
	return BuildPacket(buf, maxBufLen, dataBuf, index);
}

uint8 ParsePacket(const uint8 *buf, uint8 len, uint8 *parsedLen)
{
	static uint16 length = 0;
	static uint16 dataLeft = 0;

	static uint8 sum = 0;

	uint8 count = 0;
	
	uint8 res = PACKET_NOT_COMPLETE;

	for (uint8 i = 0; i < len; i++)
	{
		uint8 dat = *buf++;
		count++;
		
		switch (s_recvStatus)
		{
		case recv_status_idle:
			if (dat == PACKET_HEADER)
			{
				s_recvStatus = recv_status_version;
			}
			
			break;
	
		case recv_status_version:
			if (dat == PROTOCOL_VERSION)
			{
				s_recvStatus = recv_status_length;

				dataLeft = 2;
			}
			else
			{
				res = PACKET_INVALID;

				s_recvStatus = recv_status_idle;
			}
			
			break;
	
		case recv_status_length:
			length <<= 8;
			length |= dat;
			dataLeft--;
			if (dataLeft == 0)
			{
				if (length > MAX_PACKET_LEN - 1)
				{
					res = PACKET_INVALID;

					TRACE("packet invalid\r\n");
				}
				else
				{
					s_recvStatus = recv_status_data;

					dataLeft = length - 1;

					s_dataRecvIndex = 0;

					sum = 0x00;
				}
			}
			
			break;

		case recv_status_data:
			s_recvBuf[s_dataRecvIndex++] = dat;
			dataLeft--;
			sum ^= dat;
			
			if (dataLeft == 0)
			{
				s_recvStatus = recv_status_sum;
			}
	
			break;

		case recv_status_sum:
			if (sum == dat)
			{
				res = PACKET_OK;

				TRACE("packet OK\r\n");
			}
			else
			{
				res = PACKET_INVALID;

				TRACE("packet invalid\r\n");
			}

			s_recvStatus = recv_status_idle;
			
			break;
			
		}

		if (res == PACKET_OK || res == PACKET_INVALID)
		{
			break;
		}
	}

	if (parsedLen != NULL)
	{
		*parsedLen = count;
	}
	
	return res;
}

void ResetParsePacket()
{
	s_recvStatus = recv_status_idle;
	
	s_dataRecvIndex = 0;
}

const uint8 *GetPacketData()
{
	return s_recvBuf;
}

uint8 GetPacketType()
{
	return s_recvBuf[0];
}

uint8 GetPacketDataLen()
{
	return s_dataRecvIndex;
}

bool ParseGetMesureDataPacket(const uint8 * data, uint8 len, uint8 *dataMap)
{
	if (len != SIZE_GET_MEASURE_DATA)
	{
		return false;
	}

	uint8 index = 0;

	uint8 dat;

	index++;
	dat = data[index++];
	if (dataMap != NULL)
	{
		*dataMap = dat;
	}
	
	return true;
}

bool ParseStartCalibrationPacket(const uint8 *data, uint8 len, uint8 *type, uint8 *start)
{
	if (len != SIZE_START_CALIBRATION)
	{
		return false;
	}

	uint8 index = 0;

	uint8 dat;

	index++;

	dat = data[index++];
	if (type != NULL)
	{
		*type = dat;
	}
	
	dat = data[index++];
	if (start != NULL)
	{
		*start = dat;
	}
	
	return true;
}

bool ParseTemperatureCalibratePacket(const uint8 *data, uint8 len, uint16 *temperature)
{
	if (len != SIZE_TEMP_CALIBRATE)
	{
		return false;
	}

	uint16 val;
	
	uint8 index = 0;
	index++;

	val = data[index++];
	val <<= 8;
	val |= data[index++];
	if (temperature != NULL)
	{
		*temperature = val;
	}
	
	return true;
}

bool ParsePressureCalibratePacket(const uint8 *data, uint8 len, uint16 *kPa)
{
	if (len != SIZE_PRESSURE_CALIBRATE)
	{
		return false;
	}

	uint16 val;
	
	uint8 index = 0;
	index++;

	val = data[index++];
	val <<= 8;
	val |= data[index++];
	if (kPa != NULL)
	{
		*kPa = val;
	}
	
	return true;
}

bool ParseTestPacket(const uint8 *data, uint8 len, uint8 *dataMap, uint16 *testData)
{
	uint8 index = 0;
	index++;

	uint16 val;
	uint8 map = data[index++];
	if (map & DATA_TEMPERATURE_MASK)
	{
		val = (data[index++] << 8);
		val |= data[index++];
		if (testData != NULL)
		{
			*testData = val;
			testData++;
		}
	}

	if (map & DATA_PRESSURE_MASK)
	{
		val = (data[index++] << 8);
		val |= data[index++];
		if (testData != NULL)
		{
			*testData = val;
			testData++;
		}
	}

	if (map & DATA_VOLTAGE_MASK)
	{
		val = (data[index++] << 8);
		val |= data[index++];
		if (testData != NULL)
		{
			*testData = val;
			testData++;
		}
	}

	if (dataMap != NULL)
	{
		*dataMap = map;
	}

	return true;
}

bool ParseSetBleNamePacket(const uint8 *data, uint8 len, uint8 name[BLE_NAME_LEN])
{
	if (len != SIZE_SET_BLE_NAME)
	{
		return false;
	}

	for (uint8 i = 0; i < BLE_NAME_LEN; i++)
	{
		name[i] = data[1 + i];
	}
	
	return true;
}

