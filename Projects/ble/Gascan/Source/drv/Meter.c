#include "comdef.h"
#include "hal_types.h"

#include "hal_adc.h"

#include "Parameter.h"

#include "TMP75.h"

#include "Meter.h"

#include "npi.h"

//125mV
#define REF_VOLTAGE      124ul

uint16 GetBatteryVoltage()
{
	uint16 adc = HalAdcRead(HAL_ADC_CHN_AIN4, HAL_ADC_RESOLUTION_12);

	//uint16 res = adc * REF_VOLTAGE / 2048 * (1220 / 220);
	//r = 1M & 220k
	uint16 res = adc * 61 * REF_VOLTAGE / 2048 / 11;

	//TRACE("bat voltage:%d.%02dV\r\n", res / 100, res % 100);
	
	return res;

	/*
	uint32 adc = HalAdcRead(HAL_ADC_CHN_VDD3, HAL_ADC_RESOLUTION_12);
	uint16 res = adc * REF_VOLTAGE * 3 / 2048;

	return res;*/
	
}

uint16 GetTemperature()
{
	uint16 res = INVALID_TEMPERATURE;
	
	uint16 tempValue;
	if (GetTMP75Temperature(&tempValue))
	{
		TRACE("temperature:0x%04X\r\n", tempValue);
		bool sign = false;
		if (tempValue & 0x8000)
		{
			sign = true;

			tempValue = ~tempValue + 1;
		}

		res = tempValue * 10 / 16; //12bit

		TRACE("temp:%d\r\n", res);
		if (sign)
		{
			res |= (1u << 15);
		}
	}

	return res;
}

