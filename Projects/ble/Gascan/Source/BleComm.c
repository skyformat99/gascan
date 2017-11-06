#include "bcomdef.h"
#include "OSAL.h"

#if defined ( PLUS_BROADCASTER )
  #include "peripheralBroadcaster.h"
#else
  #include "peripheral.h"
#endif

#include "Parameter.h"

#include "Pressure.h"

#include "Meter.h"
#include "Indicator.h"

#include "gascanGATTprofile.h"
#include "Gascan.h"
#include "Packet.h"

#include "BleComm.h"

#include "npi.h"

#define PARSE_PACKET_TIMEOUT              5000ul

typedef enum
{
	status_calibration_idle = 0,

	status_temperature_calibration_ready,

	status_pressure_calibration_ready,
	
	status_pressure_calibrating,
	
} CalibrationStatus;

static CalibrationStatus s_caliStatus = status_calibration_idle;

//for pressure calibration
static struct PressureCalibrationItem s_caliItem[MAX_CALIBRATION_COUNT];
static uint8 s_caliItemCount = 0;

//for temperature calibration
static struct TemperatureCalirationItem s_temperatureCaliItem;

// whether needs update parameter
static bool needUpdateParam = false;

void SetNeedUpdateParam(bool need)
{
	needUpdateParam = need;
}

bool GetNeedUpdateParam()
{
	return needUpdateParam;
}

void BleDisconnected()
{
	StopMeasure();
				
	StopGascanTimer(GASCAN_CONNECTED_PEROID_EVT);

	s_caliStatus = status_calibration_idle;

	//disable ad first
	uint8 initial_advertising_enable = FALSE;    
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &initial_advertising_enable );//�ع㲥    
        
    StartGascanTimer(GASCAN_UPDATE_SCAN_RSP_DATA_EVT, 0, false);   
}

static void SendBleData(uint8 *buf, uint8 len)
{
#if 0
#ifdef DEBUG_PRINT
	TRACE("send ble data %d bytes:", len);
	for (uint8 i = 0; i < 1; i++)
	{
		TRACE("0x%02X ", buf[i]);
	}
	TRACE("\r\n");
#endif
#endif

	uint8 segCnt = (len + 20 - 1) / 20;

	uint8 left = segCnt * 20 - len;

	for (uint8 i = 0; i < left; i++)
	{
		buf[len + i] = 0x00;
	}
	
	for (uint8 i = 0; i < segCnt; i++)
	{	
		GascanProfile_Notify(buf + i * 20, 20);
	}
}

//data map
static uint8 s_dataMap = 0x00;

static void MeasureCallback(uint16 kPa)
{
	uint8 buf[MAX_PACKET_LEN];
	uint16 data[3];
	uint8 index = 0;
	
	if (s_dataMap & DATA_TEMPERATURE_MASK)
	{
		uint16 temp = GetTemperature();
		if (temp & (1u << 15))
		{
			//minus
			TRACE("-%dC\r\n", temp & ~(1u << 15));
		}
		else
		{
			TRACE("%dC\r\n", temp);
		}
		
		data[index++] = temp;
	}

	if (s_dataMap & DATA_PRESSURE_MASK)
	{
		TRACE("%dkPa\r\n", kPa);
		data[index++] = kPa;
	}

	if (s_dataMap & DATA_VOLTAGE_MASK)
	{
		uint16 vol = GetBatteryVoltage();
		TRACE("%d.%02dV\r\n", vol / 100, vol % 100);
		data[index++] = vol;
	}
	
	uint8 len = BuildMeasureDataPacket(buf, sizeof(buf), RESULT_OK, REASON_NONE, s_dataMap, data);
	s_dataMap = 0x00;
	
	SendBleData(buf, len);
}

static void ProcessMeasurePacket(uint8 dataMap)
{
	uint8 result = RESULT_OK;
	uint8 reason = REASON_NONE;

	uint8 buf[MAX_PACKET_LEN];
	uint8 len;

	if (s_caliStatus == status_temperature_calibration_ready)
	{
		//temperature calibration already started
		result = RESULT_FAILED;

		reason = REASON_TEMP_CALIBRATE_STARTED;
	}
	else if (s_caliStatus == status_pressure_calibration_ready)
	{
		//pressure calibration started
		result = RESULT_FAILED;

		reason = REASON_PRESSURE_CALIBRATE_STARTED;
	}
	else if (s_caliStatus == status_pressure_calibrating)
	{
		//pressure calibration is being done
		result = RESULT_FAILED;

		reason = REASON_PRESSURE_CALIBRATION_BEING_DONE;
	}
			
	if (dataMap & DATA_TEMPERATURE_MASK)
	{
		if (g_tempCaliItem.adc == 0)
		{
			//temperature not calibrated
			result = RESULT_FAILED;

			reason = REASON_TEMP_NOT_CALIBRATED;
		}
	}
	
	if (dataMap & DATA_PRESSURE_MASK)
	{
		if (g_pressureCaliItemCount == 0)
		{
			result = RESULT_FAILED;
	
			reason = REASON_PRESSURE_NOT_CALIBRATED;
		}
	}

	if (result == RESULT_OK)
	{
		s_dataMap = dataMap;
				
		TRACE("start measure\r\n");
		StartMeasure(MeasureCallback);
	}
	else
	{
		TRACE("result:%d,reason:%d\r\n", result, reason);
			
		len = BuildMeasureDataPacket(buf, sizeof(buf), result, reason, dataMap, NULL);
		SendBleData(buf, len);
	}
}

static void ProcessStartCalibration(uint8 type, uint8 start)
{
	uint8 result;

	uint8 reason;

	if (type == CALIBRATION_TYPE_TEMP)
	{
		//temperature calibration
		if (start == CALIBRATION_START)
		{
			if (s_caliStatus == status_temperature_calibration_ready)
			{
				//temperature calibration already started
				result = RESULT_FAILED;
	
				reason = REASON_TEMP_CALIBRATE_STARTED;
			}
			else if (s_caliStatus == status_pressure_calibration_ready)
			{
				//pressure calibration already started
				result = RESULT_FAILED;
	
				reason = REASON_PRESSURE_CALIBRATE_STARTED;
			}
			else if (s_caliStatus == status_pressure_calibrating)
			{
				//pressure calibration is being done
				result = RESULT_FAILED;
	
				reason = REASON_PRESSURE_CALIBRATION_BEING_DONE;
			}
			else
			{
				//idle
				result = RESULT_OK;
				
				reason = REASON_NONE;
				
				s_caliStatus = status_temperature_calibration_ready;
			}
		}
		else
		{
			if (s_caliStatus == status_pressure_calibration_ready)
			{
				result = RESULT_FAILED;
	
				reason = REASON_PRESSURE_CALIBRATE_STARTED;
			}
			else if (s_caliStatus == status_pressure_calibrating)
			{
				result = RESULT_FAILED;
	
				reason = REASON_PRESSURE_CALIBRATION_BEING_DONE;
			}
			else if (s_caliStatus == status_calibration_idle)
			{
				result = RESULT_FAILED;
	
				reason = REASON_TEMP_CALIBRATE_NOT_STARTED;
			}
			else
			{
				g_tempCaliItem = s_temperatureCaliItem;
					
				result = RESULT_OK;
				reason = REASON_NONE;
	
				SetTemperatureCaliItem(&g_tempCaliItem);
	
				needUpdateParam = true;
	
				s_caliStatus = status_calibration_idle;
			}
		}
	}
	else if (type == CALIBRATION_TYPE_PRESSURE)
	{
		//pressure calibration
		if (start == CALIBRATION_START)
		{
			if (s_caliStatus == status_temperature_calibration_ready)
			{
				result = RESULT_FAILED;
	
				reason = REASON_TEMP_CALIBRATE_STARTED;
			}
			else if (s_caliStatus == status_pressure_calibration_ready)
			{
				result = RESULT_FAILED;
	
				reason = REASON_PRESSURE_CALIBRATE_STARTED;
			}
			else if (s_caliStatus == status_pressure_calibrating)
			{
				result = RESULT_FAILED;
	
				reason = REASON_PRESSURE_CALIBRATION_BEING_DONE;
			}
			else
			{
				result = RESULT_OK;
				
				reason = REASON_NONE;
	
				s_caliItemCount = 0;
				
				s_caliStatus = status_pressure_calibration_ready;
			}
		}
		else
		{
			if (s_caliStatus == status_temperature_calibration_ready)
			{
				result = RESULT_FAILED;
	
				reason = REASON_TEMP_CALIBRATE_NOT_STARTED;
			}
			else if (s_caliStatus == status_pressure_calibrating)
			{
				result = RESULT_FAILED;
	
				reason = REASON_PRESSURE_CALIBRATION_BEING_DONE;
			}
			else if (s_caliStatus == status_calibration_idle)
			{
				result = RESULT_FAILED;
	
				reason = REASON_PRESSURE_CALIBRATE_NOT_STARTED;
			}
			else
			{
				if (s_caliItemCount == MAX_CALIBRATION_COUNT)
				{
					//save
					for (uint8 i = 0; i < MAX_CALIBRATION_COUNT; i++)
					{	
						g_pressureCaliItem[i] = s_caliItem[i];
					}
					g_pressureCaliItemCount = s_caliItemCount;
					
					s_caliItemCount = 0;
					
					result = RESULT_OK;
					reason = REASON_NONE;
	
					SetCalibration(g_pressureCaliItem, g_pressureCaliItemCount);
	
					needUpdateParam = true;
				}
				else
				{
					result = RESULT_FAILED;
				
					reason = REASON_PRESSURE_CALIBRATION_POINT_NUM_WRONG;
				}
	
				s_caliStatus = status_calibration_idle;
			}
		}
	}
	else
	{
		result = RESULT_FAILED;

		reason = REASON_OTHER;
	}

	uint8 buf[MAX_PACKET_LEN];

	TRACE("result:%d,reason:%d\r\n", result, reason);
	uint8 len = BuildStartCalibrationRetPacket(buf, sizeof(buf), type, start, result, reason);
	SendBleData(buf, len);
}

static void PressureCalibrateCallback(uint16 kPa, uint32 adc)
{	
	s_caliItem[s_caliItemCount].kPa = kPa;
	s_caliItem[s_caliItemCount].adc = adc;
	s_caliItemCount++;

	TRACE("pressure cali,kPa:%d,adc:0x%08lX\r\n", kPa, adc);
	
	s_caliStatus = status_pressure_calibration_ready;
	
	uint8 buf[MAX_PACKET_LEN];
	
	uint8 len = BuildPressureCalibrateRetPacket(buf, sizeof(buf), kPa, RESULT_OK, REASON_NONE);
	SendBleData(buf, len);
}

static void ProcessTemperatureCalibrate(uint16 temperature)
{
	uint8 result;

	uint8 reason;
	
	TRACE("temp calibration:%dC\r\n", temperature);
	
	if (s_caliStatus == status_calibration_idle)
	{
		result = RESULT_FAILED;
		
		reason = REASON_TEMP_CALIBRATE_NOT_STARTED;
	}
	else if (s_caliStatus == status_pressure_calibration_ready)
	{
		result = RESULT_FAILED;
		
		reason = REASON_PRESSURE_CALIBRATE_STARTED;
	}
	else if (s_caliStatus == status_pressure_calibrating)
	{
		result = RESULT_FAILED;

		reason = REASON_PRESSURE_CALIBRATION_BEING_DONE;
	}
	else
	{
		result = RESULT_OK;
			
		reason = REASON_NONE;

		uint16 tempAdc = GetTemperatureAdc();

		s_temperatureCaliItem.adc = tempAdc;
		s_temperatureCaliItem.temperature = temperature;
	}
	
	uint8 buf[MAX_PACKET_LEN];

	TRACE("result:%d,reason:%d\r\n", result, reason);
		
	uint8 len = BuildTemperatureCalibrateRetPacket(buf, sizeof(buf), temperature, result, reason);
	SendBleData(buf, len);
}

static void ProcessPressureCalibrate(uint16 kPa)
{
	uint8 result;

	uint8 reason;
	
	if (s_caliStatus == status_calibration_idle)
	{
		result = RESULT_FAILED;
		
		reason = REASON_PRESSURE_CALIBRATE_NOT_STARTED;
	}
	else if (s_caliStatus == status_temperature_calibration_ready)
	{
		result = RESULT_FAILED;
		
		reason = REASON_TEMP_CALIBRATE_STARTED;
	}
	else if (s_caliStatus == status_pressure_calibrating)
	{
		result = RESULT_FAILED;

		reason = REASON_PRESSURE_CALIBRATION_BEING_DONE;
	}
	else
	{
		result = RESULT_OK;
			
		reason = REASON_NONE;

		s_caliStatus = status_pressure_calibrating;
		
		StartCalibrate(PressureCalibrateCallback, kPa);
	}
	
	if (result != RESULT_OK)
	{
		uint8 buf[MAX_PACKET_LEN];

		TRACE("result:%d,reason:%d\r\n", result, reason);
		
		uint8 len = BuildPressureCalibrateRetPacket(buf, sizeof(buf), kPa, result, reason);
		SendBleData(buf, len);
	}
}

static void ProcessSetBleName(const uint8 name[BLE_NAME_LEN])
{
	osal_memcpy(g_bleName, name, BLE_NAME_LEN);
	needUpdateParam = true;

	//update ble name
	UpdateBleName(g_bleName);
                             
	uint8 buf[MAX_PACKET_LEN];

	uint8 len = BuildSetBleNameRetPacket(buf, sizeof(buf), RESULT_OK);
	SendBleData(buf, len);
}

static void ProcessTestPacket(uint8 dataMap, const uint16 *testData)
{
	uint8 buf[MAX_PACKET_LEN];

	uint8 len = BuildTestMeasureDataPacket(buf, sizeof(buf), RESULT_OK, REASON_NONE, dataMap, testData);
	SendBleData(buf, len);
}

void ProcessBleCom(const uint8 *buf, uint8 len)
{
	//for echo test
	//GascanProfile_Notify(buf, len);
	//ProcessSetBleName("zhuwenhui");
#if 1
	uint8 parsedLen;

	uint8 res = ParsePacket(buf, len, &parsedLen);
	if (res == PACKET_OK)
	{
		StopGascanTimer(GASCAN_PARSE_PACKET_TIMEOUT_EVT);
		
		uint8 type = GetPacketType();
		uint8 dataLen = GetPacketDataLen();
		const uint8 *data = GetPacketData();

		TRACE("type:%d,dataLen:%d\r\n", type, dataLen);
		
		switch (type)
		{
		case TYPE_MEASURE:
			{
				uint8 dataMap;
				if (ParseGetMesureDataPacket(data, dataLen, &dataMap))
				{
					TRACE("parse get measure data ok, map:0x%02X\r\n", dataMap);

					ProcessMeasurePacket(dataMap);
				}
				else
				{
					
					TRACE("parse get measure data failed\r\n");
				}
			}

			break;

		case TYPE_START_CALIBRATION:
			{
				uint8 type;
				uint8 start;
				if (ParseStartCalibrationPacket(data, dataLen, &type, &start))
				{
					TRACE("parse start calibration ok, type:%d,start:0x%02X\r\n", type, start);

					ProcessStartCalibration(type, start);
				}
				else
				{
					TRACE("parse start calibration failed\r\n");
				}
			}
			
			break;

		case TYPE_TEMP_CALIBRATE:
			{
				uint16 temperature;
				if (ParseTemperatureCalibratePacket(data, dataLen, &temperature))
				{
					TRACE("parse temp calibrate ok,%dC\r\n", temperature);

					ProcessTemperatureCalibrate(temperature);
				}
				else
				{
					TRACE("parse temp calibrate failed\r\n");
				}
			}
			
			break;

		case TYPE_PRESSURE_CALIBRATE:
			{
				uint16 kPa;
				if (ParsePressureCalibratePacket(data, dataLen, &kPa))
				{
					TRACE("parse pressure calibrate ok,%dkPa\r\n", kPa);

					ProcessPressureCalibrate(kPa);
				}
				else
				{
					TRACE("parse pressure calibrate failed\r\n");
				}
			}
			
			break;
			
		case TYPE_SET_BLE_NAME:
			{
				uint8 name[BLE_NAME_LEN];
				if (ParseSetBleNamePacket(data, dataLen, name))
				{
					TRACE("parse set ble name ok:%s\r\n", name);

					ProcessSetBleName(name);
				}
				else
				{
					TRACE("parse set ble name failed\r\n");
				}
			}
			
			break;
			
		case TYPE_TEST:
			{
				uint8 dataMap;
				uint16 testData[3];
				if (ParseTestPacket(data, dataLen, &dataMap, testData))
				{
					TRACE("parse test data ok, map:0x%02X\r\n", dataMap);
					
					ProcessTestPacket(dataMap, testData);
				}
				else
				{
					
					TRACE("parse test data failed\r\n");
				}
			}

			break;
		}
	}
	else if (res == PACKET_INVALID)
	{
		TRACE("invalid packet\r\n");
		StopGascanTimer(GASCAN_PARSE_PACKET_TIMEOUT_EVT);
	}
	else
	{
		//incomplete
		TRACE("incomplete packet\r\n");
		StartGascanTimer(GASCAN_PARSE_PACKET_TIMEOUT_EVT, PARSE_PACKET_TIMEOUT, false);
	}
	
#endif
}

