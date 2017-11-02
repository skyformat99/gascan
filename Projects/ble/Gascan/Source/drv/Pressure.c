#include "comdef.h"
#include "hal_types.h"

#include "Hx711.h"

#include "Parameter.h"
#include "Pressure.h"

#include "npi.h"

typedef enum
{
	pressure_status_idle = 0,

	pressure_status_calibrating,

	pressure_status_measuring,
} PressureStatus;

static PressureStatus s_measureStatus = pressure_status_idle;

//weigh
static void (*s_measureCallback)(uint16) = NULL;

//calibrate
#define CALIBRATE_SAMPLE_COUNT     10

static void (*s_calibrateCallback)(uint16, uint32) = NULL;

static uint16 s_kPa = 0;

static uint8 s_caliSampleCount = 0;
static uint32 s_adcSum = 0;
static uint32 s_adcMax = 0;
static uint32 s_adcMin = 0;

static const struct PressureCalibrationItem *s_caliItem = NULL;
static uint8 s_caliItemCount = 0;

void InitPressure()
{
	InitHx711();
}

void SetCalibration(const struct PressureCalibrationItem *caliItem, uint8 caliItemCount)
{
	s_caliItem = caliItem;
	s_caliItemCount = caliItemCount;
}

static void AdcCallback(uint32 adc)
{
	//TRACE("adc:0x%08lX\r\n", adc);

	switch (s_measureStatus)
	{
	case pressure_status_calibrating:
		s_adcSum += adc;
		if (s_adcMax < adc)
		{
			s_adcMax = adc;
		}

		if (s_adcMin > adc)
		{
			s_adcMin = adc;
		}
		
		s_caliSampleCount--;
		if (s_caliSampleCount == 0)
		{
			StopHx711();
			
			s_adcSum -= s_adcMax + s_adcMin;

			s_adcSum /= (CALIBRATE_SAMPLE_COUNT - 2);

			if (s_calibrateCallback != NULL)
			{
				s_calibrateCallback(s_kPa, s_adcSum);

				s_calibrateCallback = NULL;
				
				s_measureStatus = pressure_status_idle;
			}
		}
		
		break;

	case pressure_status_measuring:
		{
			uint32 val;
			uint16 kPa;

			//StopHx711();
			
			if (s_caliItemCount == 3)
			{	
				if (adc >= s_caliItem[1].adc)
				{
					val = (s_caliItem[2].kPa - s_caliItem[1].kPa) * (adc - s_caliItem[1].adc);
								
					kPa = val / (s_caliItem[2].adc - s_caliItem[1].adc) + s_caliItem[1].kPa;
				}
				else if (adc >= s_caliItem[0].adc)
				{
					val = (s_caliItem[1].kPa - s_caliItem[0].kPa) * (adc - s_caliItem[0].adc);

					kPa = val / (s_caliItem[1].adc - s_caliItem[0].adc) + s_caliItem[0].kPa;
				}
				else
				{
					val = (s_caliItem[1].kPa - s_caliItem[0].kPa) * (s_caliItem[0].adc - adc);

					if (s_caliItem[0].kPa >= val / (s_caliItem[1].adc - s_caliItem[0].adc))
					{
						kPa = s_caliItem[0].kPa - val / (s_caliItem[1].adc - s_caliItem[0].adc);
					}
					else
					{
						kPa = 0;
					}
				}
				
				if (s_measureCallback != NULL)
				{
					s_measureCallback(kPa);
				}

				s_measureCallback = NULL;

				s_measureStatus = pressure_status_idle;
			}
		}
		
		break;
	}
}

void StartMeasure(void (*callback)(uint16 kPa))
{
	s_measureCallback = callback;
	
	StartHx711(AdcCallback, Channel_A_64, false);

	s_measureStatus = pressure_status_measuring;
}

void StartCalibrate(void (*callback)(uint16 kPa, uint32 adc), uint16 kPa)
{
	s_calibrateCallback = callback;
	s_kPa = kPa;
	
	s_caliSampleCount = CALIBRATE_SAMPLE_COUNT;

	s_adcSum = 0;
	s_adcMax = 0;
	s_adcMin = 0xffffffff;
	
	StartHx711(AdcCallback, Channel_A_64, true);

	s_measureStatus = pressure_status_calibrating;
}

void StopMeasure()
{
	s_measureCallback = NULL;
	
	StopHx711();
	
	s_measureStatus = pressure_status_idle;
}

