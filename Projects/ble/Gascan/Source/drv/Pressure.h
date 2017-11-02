
#ifndef _PRESSURE_H_
#define _PRESSURE_H_

struct PressureCalibrationItem;

extern void InitPressure();

extern void StartMeasure(void (*callback)(uint16 kPa));

extern void SetCalibration(const struct PressureCalibrationItem *caliItem, uint8 caliItemCount);

extern void StartCalibrate(void (*callback)(uint16 kPa, uint32 adc), uint16 kPa);

extern void StopMeasure();

#endif

