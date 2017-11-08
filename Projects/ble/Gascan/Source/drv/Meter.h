
#ifndef _METER_H_
#define _METER_H_

//mV
#define  LOW_POWER_THRESHOLD       280ul

struct TemperatureCalirationItem;


//unit of returned value is 100mV
extern uint16 GetBatteryVoltage();

//unit of returned value is 0.1C
extern void SetTemperatureCaliItem(const struct TemperatureCalirationItem *caliItem);

extern uint16 GetTemperature();

extern uint16 GetTemperatureAdc();

#endif

