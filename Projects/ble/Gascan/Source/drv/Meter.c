#include "comdef.h"
#include "hal_types.h"

#include "hal_adc.h"

#include "Parameter.h"

#include "Meter.h"

#include "npi.h"

//125mV
#define REF_VOLTAGE      125ul

static const struct TemperatureCalirationItem *s_temperatureCaliItem = NULL;

uint16 GetBatteryVoltage()
{
	uint16 adc = HalAdcRead(HAL_ADC_CHN_AIN4, HAL_ADC_RESOLUTION_12);

	//uint16 res = adc * REF_VOLTAGE / 2048 * (1220 / 220);
	//r = 1M & 220k
	uint16 res = adc * 61 * REF_VOLTAGE / 2048 / 11;

	//TRACE("bat voltage:%d.%02dV\r\n", res / 100, res % 100);
	
	return res;
}

void SetTemperatureCaliItem(const struct TemperatureCalirationItem *caliItem)
{
	s_temperatureCaliItem = caliItem;
}

uint16 GetTemperature()
{
	uint32 res = HalAdcRead(HAL_ADC_CHN_TEMP, HAL_ADC_RESOLUTION_12);

	TRACE("temp adc:%d\r\n", res);	
	TRACE("temp cali adc:%d\r\n", s_temperatureCaliItem->adc);
	
	//original formula
	//val = (x - 1480) / 4.5 + 125 - 100
	//res *= 100;
	//res = (res - 91750) / 45;
	
	//calibrated formula
	//val = (res - s_temperatureCaliItem->adc) / 4.5 + 100 + s_temperatureCaliItem.temperature - 100;
	res *= 100ul;
	res += (100 * 10 + s_temperatureCaliItem->temperature) * 45ul;
	res -= s_temperatureCaliItem->adc * 100ul;
	res /= 45ul;
	
	if (res >= 1000)
	{
		//positive
		res -= 1000;
	}
	else
	{	
		//negative
		res = (1000 - res) | (1u << 15);
	}

	return (uint16)res;
}

uint16 GetTemperatureAdc()
{
	uint16 res = HalAdcRead(HAL_ADC_CHN_TEMP, HAL_ADC_RESOLUTION_12);

	return res;
}

