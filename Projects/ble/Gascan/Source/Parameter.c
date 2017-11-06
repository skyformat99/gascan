#include "hal_types.h"
#include "comdef.h"

#include "EEPROM.h"
#include "Parameter.h"

#include "npi.h"

#define  HEADER     0xA5

#define  PARAMETER_START_ADDR      0

StoreVector g_storeVector;

//for test
#define ADC_0G        0x7F6140

#define ADC_50G       0x80D5E5

static const Parameter s_defaultParameter = 
{
	.pressureCaliItem = 
	{
		[0] = 
		{
			.kPa = 0,
			
			.adc = ADC_0G
		},

		[1] = 
		{
			.kPa = 0,
			
			.adc = ADC_50G
		},

		[2] = 
		{
			.kPa = 0,
			
			.adc = 0
		},
	},

	.pressureCaliItemCount = 0,

	.tempCaliItem = 
	{
		.temperature = 250,

		.adc = 0 /*1480*/
	},

	.bleName = 
	{
		'G',  'a',  's',  'c',  'a',  'n',  '\0', '\0', '\0', '\0', 
		'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', 
	},
};

bool LoadParameter()
{
	uint16 addr = PARAMETER_START_ADDR;

	uint8 dat;	
	//header
	if (!ReadEEPROM(addr, &dat))
	{		
		return false;
	}

	if (dat != HEADER)
	{	
		return false;
	}

	//size
	uint16 size;
	addr += sizeof(dat);
	if (!ReadEEPROMData(addr, (uint8 *)&size, sizeof(size), NULL))
	{		
		return false;
	}

	if (size != sizeof(Parameter))
	{		
		return false;
	}

	//parameter
	addr += sizeof(size);
	if (!ReadEEPROMData(addr, (uint8 *)&g_storeVector.parameter, size, NULL))
	{
		return false;
	}

	//sum
	uint8 sum = 0x00;
	uint8 *p = (uint8 *)&g_storeVector.parameter;
	for (int i = 0; i < size; i++)
	{
		sum ^= *p++;
	}
	
	addr += size;
	if (!ReadEEPROM(addr, &dat))
	{		
		return false;
	}
	
	if (sum != dat)
	{		
		return false;
	}

	TRACE("load parameter OK\r\n");
	
	return true;
}

void LoadDefaultParameter()
{
	g_storeVector.parameter = s_defaultParameter;
}

bool SaveParameter()
{
  	uint16 addr = PARAMETER_START_ADDR;

	g_storeVector.header = HEADER;
	g_storeVector.size = sizeof(g_storeVector.parameter);

	uint8 *p = (uint8 *)&g_storeVector.parameter;
	uint8 sum = 0x00;
	for (int i = 0; i < sizeof(g_storeVector.parameter); i++)
	{	
		sum ^= *p++;
	}

	g_storeVector.sum = sum;

	if (!WriteEEPROMData(addr, (uint8 *)&g_storeVector, sizeof(g_storeVector)))
	{
		return false;
	}

	TRACE("save parameter OK\r\n");
	
	return true;

	/*
	//header
	dat = HEADER;
	if (!WriteEEPROM(addr, dat))
	{
		return false;
	}

	//size
	addr += sizeof(dat);
	uint16 size = sizeof(g_storeVector.parameter);
	if (!WriteEEPROMData(addr, (uint8 *)&size, sizeof(size)))
	{
		ret = false;
	}

	//parameter
	addr += sizeof(size);
	if (!WriteEEPROMData(addr, (uint8 *)&g_storeVector.parameter, sizeof(g_storeVector.parameter)))
	{
		return false;
	}
	
	uint8 *p = (uint8 *)&g_storeVector.parameter;
	int i;
	uint8 sum = 0x00;
	for (i = 0; i < size; i++)
	{	
		sum ^= *p++;
	}

	addr += size;
	if (!WriteEEPROM(addr, sum))
	{
		return false;
	}
	
	TRACE("save parameter OK\r\n");
	
	return true;
	*/
}

