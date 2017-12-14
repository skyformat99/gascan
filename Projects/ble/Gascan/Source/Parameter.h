
#ifndef _PARAMETER_H_
#define _PARAMETER_H_

#include "Protocol.h"

#define MAX_CALIBRATION_COUNT   3

struct PressureCalibrationItem
{
	uint16 kPa;

	uint32 adc;
};

typedef struct 
{
	struct PressureCalibrationItem pressureCaliItem[MAX_CALIBRATION_COUNT];
	uint8  pressureCaliItemCount;
	
	uint8 bleName[BLE_NAME_LEN];
	
} Parameter;


typedef struct
{
	uint8  header;

	uint16 size;

	Parameter parameter;

	uint8  sum;
} StoreVector;

extern StoreVector g_storeVector;

#define g_pressureCaliItem           g_storeVector.parameter.pressureCaliItem
#define g_pressureCaliItemCount      g_storeVector.parameter.pressureCaliItemCount
#define g_bleName                    g_storeVector.parameter.bleName

extern bool LoadParameter();
extern void LoadDefaultParameter();

extern bool SaveParameter();

#endif


