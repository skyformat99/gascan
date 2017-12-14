
#ifndef _METER_H_
#define _METER_H_

//mV
#define  LOW_POWER_THRESHOLD       280ul

//unit of returned value is 100mV
extern uint16 GetBatteryVoltage();

#define INVALID_TEMPERATURE      0xffff

extern uint16 GetTemperature();

#endif

