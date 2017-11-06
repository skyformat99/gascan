
#ifndef _PARAMETER_H_
#define _PARAMETER_H_

#include "Protocol.h"

#define FIRMWARE_VER_LEN  8
#define FIRMWARE_VER      "0.00.001"

#define WEB_SITE          "https://pcb-layout.taobao.com"

#define MAX_CALIBRATION_COUNT   3

struct PressureCalibrationItem
{
	uint16 kPa;

	uint32 adc;
};

struct TemperatureCalirationItem
{
	uint16 temperature;

	uint16 adc;
};

typedef struct 
{
	struct PressureCalibrationItem pressureCaliItem[MAX_CALIBRATION_COUNT];
	uint8  pressureCaliItemCount;

	struct TemperatureCalirationItem tempCaliItem;

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
#define g_tempCaliItem               g_storeVector.parameter.tempCaliItem
#define g_bleName                    g_storeVector.parameter.bleName

extern bool LoadParameter();
extern void LoadDefaultParameter();

extern bool SaveParameter();

#endif


