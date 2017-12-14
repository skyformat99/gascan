#include "comdef.h"
#include "hal_types.h"

#include "I2C.h"

#include "I2CDevice.h"

bool DetectI2CDevice(uint8 dev)
{
	I2CStart();
			
	WriteI2CByte(dev);
	bool res = CheckI2CAck();

	I2CStop();

	return res;
}

bool WriteI2CDeviceData(uint8 dev, const uint8 *addr, uint8 addrLen, const uint8 *pData, uint16 len)
{
	I2CStart();
		
	WriteI2CByte(dev);
	if (!CheckI2CAck())
	{
		I2CStop();
		return false;
	}

	while (addrLen--)
	{
		WriteI2CByte(*addr++);
		if (!CheckI2CAck())
		{
			I2CStop();
			return false;
		}
	}
	
	while (len--)
	{
		WriteI2CByte(*pData++);
		if (!CheckI2CAck())
		{
			I2CStop();
			return false;
		}
	}
	
	I2CStop();

	return true;
}

bool ReadI2CDeviceData(uint8 dev, const uint8 *addr, uint8 addrLen, uint8 *pData, uint16 len, uint16 *pRetLen)
{
	uint16 count = 0;

	if (addrLen > 0)
	{
		I2CStart();
		
		WriteI2CByte(dev);
		if (!CheckI2CAck())
		{
			I2CStop();
			return false;
		}

		while (addrLen--)
		{
			WriteI2CByte(*addr++);
			if (!CheckI2CAck())
			{
				I2CStop();
				return false;
			}
		}
	}
	
	I2CStart();
		
	WriteI2CByte(dev | 0x01);
	if (!CheckI2CAck())
	{
		I2CStop();
		return false;
	}

	while (len--)
	{
		*pData++ = ReceiveI2CByte();
		SendI2CAck(len > 0);
		
		count++;
	}
		
	I2CStop();

	if (pRetLen != NULL)
	{
		*pRetLen = count;
	}
	
	return true;
}

