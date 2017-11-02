
#include "hal_led.h"
#include "Indicator.h"

void IndicateNormalPower()
{
	HalLedBlink(LED_GREEN, 1, 99, 3000);
}

void IndicateLowPower()
{
	HalLedBlink(LED_RED, 1, 99, 3000);
}

void IndicateConnectedInNormalPower()
{
	HalLedBlink(LED_GREEN, 1, 5, 1000);
}

void IndicateConnectedInLowPower()
{
	HalLedBlink(LED_RED, 1, 5, 1000);
}

