#include "comdef.h"
#include "hal_types.h"

#include "I2CDevice.h"
#include "TMP75.h"

#include "npi.h"

#define TMP75_DEV_ADDRESS   0x90

//register
#define TEMP_REG_PTR       0
#define CONFIG_REG_PTR     1
#define TLOW_REG_PTR       2
#define THIGH_REG_PTR      3

static bool s_canReadTemperature = false;

static bool EnterTMPShutdownMode()
{
	uint8 buf[2];

	buf[0] = CONFIG_REG_PTR;
	buf[1] = (1 << 0); //shut down

	s_canReadTemperature = false;
	
	return WriteI2CDeviceData(TMP75_DEV_ADDRESS, NULL, 0, buf, sizeof(buf));
}

bool InitTMP75()
{
	bool res = EnterTMPShutdownMode();
	if (res)
	{
		TRACE("init tmp75 ok\r\n");
	}
	else
	{
		TRACE("init tmp75 failed\r\n");
	}
	
	return res;
}

bool StartTMP75Conversion()
{
	uint8 buf[2];
	
	buf[0] = CONFIG_REG_PTR;
	buf[1] = (1 << 6) | (1 << 5); //12bit resolution

	s_canReadTemperature = false;
	
	bool res = WriteI2CDeviceData(TMP75_DEV_ADDRESS, NULL, 0, buf, sizeof(buf));
	if (res)
	{
		TRACE("start tmp75 OK\r\n");
	}
	else
	{
		TRACE("start tmp75 failed\r\n");
	}

	return res;
}

bool StopTMP75Conversion()
{
	bool res = EnterTMPShutdownMode();
	if (res)
	{
		TRACE("tmp75 shut down OK\r\n");
	}
	else
	{
		TRACE("tmp75 shut down failed\r\n");
	}

	return res;
}

bool GetTMP75Temperature(uint16 *tempValue)
{
	bool ret;
	
	if (!s_canReadTemperature)
	{
		uint8 ptr = TEMP_REG_PTR;

		ret = WriteI2CDeviceData(TMP75_DEV_ADDRESS, NULL, 0, &ptr, sizeof(ptr));
		if (!ret)
		{
			return false;
		}

		s_canReadTemperature = true;
	}

	uint8 buf[2];
	ret = ReadI2CDeviceData(TMP75_DEV_ADDRESS, NULL, 0, buf, sizeof(buf), NULL);
	if (ret)
	{
		uint16 temp = (buf[0] << 8) | buf[1];
		temp >>= 4;

		if (tempValue != NULL)
		{
			*tempValue = temp;
		}
	}

	return ret;
}

