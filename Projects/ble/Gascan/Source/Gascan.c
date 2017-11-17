/**************************************************************************************************
  Filename:       Gascan.c
  Revised:        $Date: 2010-08-06 08:56:11 -0700 (Fri, 06 Aug 2010) $
  Revision:       $Revision: 23333 $

  Description:    This file contains the Gascan BLE Peripheral sample application
                  for use with the CC2540 Bluetooth Low Energy Protocol Stack.

  Copyright 2010 - 2013 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include <stdio.h>

#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"
#include "osal_cbtimer.h"

#include "OnBoard.h"
#include "hal_key.h"

#include "gatt.h"

#include "hci.h"

#include "gapgattserver.h"
#include "gattservapp.h"
#include "gap.h"

#if defined ( PLUS_BROADCASTER )
  #include "peripheralBroadcaster.h"
#else
  #include "peripheral.h"
#endif

#if defined FEATURE_OAD    
  #include "oad.h"    
  #include "oad_target.h"    
#endif

#include "gascanGATTprofile.h"

#include "Parameter.h"

#include "Pressure.h"

#include "Meter.h"
#include "Indicator.h"

#include "Gascan.h"

#include "Packet.h"

#include "BleComm.h"

#include "Npi.h"

#define POWER_ON_DELAY_TIME               2000ul
#define DETECT_INTERVAL_WHEN_CONNECTED    5000ul
/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

//for android
#if 0
// What is the advertising interval when device is discoverable (units of 625us, 160=100ms)
#define DEFAULT_ADVERTISING_INTERVAL          3200

// Limited discoverable mode advertises for 30.72s, and then stops
// General discoverable mode advertises indefinitely

#define DEFAULT_DISCOVERABLE_MODE             GAP_ADTYPE_FLAGS_GENERAL

// Minimum connection interval (units of 1.25ms, 80=100ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL     240

// Maximum connection interval (units of 1.25ms, 800=1000ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL     320

// Slave latency to use if automatic parameter update request is enabled
#define DEFAULT_DESIRED_SLAVE_LATENCY         0

// Supervision timeout value (units of 10ms, 1000=10s) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_CONN_TIMEOUT          600

#else
//for android & ios

#ifdef POWER_SAVING

//for andoid & ios
// What is the advertising interval when device is discoverable (units of 625us, 160=100ms)
//#define DEFAULT_ADVERTISING_INTERVAL         3200
#define DEFAULT_ADVERTISING_INTERVAL         1600


// Limited discoverable mode advertises for 30.72s, and then stops
// General discoverable mode advertises indefinitely

#define DEFAULT_DISCOVERABLE_MODE             GAP_ADTYPE_FLAGS_GENERAL

// Minimum connection interval (units of 1.25ms, 80=100ms) if aut omatic parameter update request is enabled
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL     24
//#define DEFAULT_DESIRED_MIN_CONN_INTERVAL     8

// Maximum connection interval (units of 1.25ms, 800=1000ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL     40
//#define DEFAULT_DESIRED_MAX_CONN_INTERVAL     8

// Slave latency to use if automatic parameter update request is enabled
#define DEFAULT_DESIRED_SLAVE_LATENCY         0

// Supervision timeout value (units of 10ms, 1000=10s) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_CONN_TIMEOUT          400

#else
// What is the advertising interval when device is discoverable (units of 625us, 160=100ms)
#define DEFAULT_ADVERTISING_INTERVAL          160

// Limited discoverable mode advertises for 30.72s, and then stops
// General discoverable mode advertises indefinitely

#define DEFAULT_DISCOVERABLE_MODE             GAP_ADTYPE_FLAGS_GENERAL

// Minimum connection interval (units of 1.25ms, 80=100ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL     8

// Maximum connection interval (units of 1.25ms, 800=1000ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL     8

// Slave latency to use if automatic parameter update request is enabled
#define DEFAULT_DESIRED_SLAVE_LATENCY         0

// Supervision timeout value (units of 10ms, 1000=10s) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_CONN_TIMEOUT          600

#endif

#endif

// Whether to enable automatic parameter update request when a connection is formed
#define DEFAULT_ENABLE_UPDATE_REQUEST         TRUE

// Connection Pause Peripheral time value (in seconds)
#define DEFAULT_CONN_PAUSE_PERIPHERAL         6

// Company Identifier: Texas Instruments Inc. (13)
#define TI_COMPANY_ID                         0x000D

#define INVALID_CONNHANDLE                    0xFFFF

// Length of bd addr as a string
#define B_ADDR_STR_LEN                        15

#if defined ( PLUS_BROADCASTER )
  #define ADV_IN_CONN_WAIT                    500 // delay 500 ms
#endif

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8 gascan_TaskID;   // Task ID for internal task/event processing

static gaprole_States_t gapProfileState = GAPROLE_INIT;

// GAP - SCAN RSP data (max size = 31 bytes)
static uint8 scanRspData[] =
{
	// complete name
	BLE_NAME_LEN,   // length of this data
	GAP_ADTYPE_LOCAL_NAME_COMPLETE,

	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',

	// connection interval range
	0x05,   // length of this data
	GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE,
	LO_UINT16(DEFAULT_DESIRED_MIN_CONN_INTERVAL),
	HI_UINT16(DEFAULT_DESIRED_MIN_CONN_INTERVAL),
	LO_UINT16(DEFAULT_DESIRED_MAX_CONN_INTERVAL),
	HI_UINT16(DEFAULT_DESIRED_MAX_CONN_INTERVAL),

	// Tx power level
	0x02,   // length of this data
	GAP_ADTYPE_POWER_LEVEL,
	0       // 0dBm
};

// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertisting)
static uint8 advertData[] =
{
	// Flags; this sets the device to use limited discoverable
	// mode (advertises for 30 seconds at a time) instead of general
	// discoverable mode (advertises indefinitely)
	0x02,   // length of this data
	GAP_ADTYPE_FLAGS,
	DEFAULT_DISCOVERABLE_MODE | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

	// service UUID, to notify central devices what services are included
	// in this peripheral
	0x03,   // length of this data
	GAP_ADTYPE_16BIT_MORE,      // some of the UUID's, but not all
	LO_UINT16(GASCANPROFILE_SERV_UUID),
	HI_UINT16(GASCANPROFILE_SERV_UUID),
};

// GAP GATT Attributes
static uint8 attDeviceName[GAP_DEVICE_NAME_LEN];

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void Gascan_ProcessOSALMsg(osal_event_hdr_t *pMsg);
static void peripheralStateNotificationCB(gaprole_States_t newState);

static void GascanProfileChangeCB(uint8 paramID, uint8 len);

/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapRolesCBs_t gascan_PeripheralCBs =
{
	peripheralStateNotificationCB,  // Profile State Change Callbacks
	NULL                            // When a valid RSSI is read from controller (not used by application)
};

// Gascan GATT Profile Callbacks
static gascanProfileCBs_t gascanProfileCBs =
{
	GascanProfileChangeCB    // Charactersitic value change callback
};


/*********************************************************************
 * PUBLIC FUNCTIONS
 */
static void SetBleName(const uint8 name[BLE_NAME_LEN])
{
	for (int i = 0; i < BLE_NAME_LEN; i++)
	{	
		scanRspData[2 + i] = name[i];

		attDeviceName[i] = name[i];
	}
}

void EnableBleAdvertise(bool enable)
{
	uint8 initial_advertising_enable = enable;

	GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &initial_advertising_enable);
}

/*********************************************************************
 * @fn      Gascan_Init
 *
 * @brief   Initialization function for the Gascan BLE Peripheral App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */

void Gascan_Init( uint8 task_id )
{	
	gascan_TaskID = task_id;

  	RegisterForKeys(gascan_TaskID);
	
	if (!LoadParameter())
	{
		LoadDefaultParameter();

		SaveParameter();
	}
	
	SetBleName(g_bleName);
	
	SetCalibration(g_pressureCaliItem, g_pressureCaliItemCount);
	SetTemperatureCaliItem(&g_tempCaliItem);
	TRACE("temp:%d,cali adc:%d\r\n", g_tempCaliItem.temperature, g_tempCaliItem.adc);
		
  	hciStatus_t hci_status = HCI_EXT_SetTxPowerCmd(HCI_EXT_TX_POWER_4_DBM);

  	HCI_EXT_HaltDuringRfCmd(HCI_EXT_HALT_DURING_RF_DISABLE);
  
  	// Setup the GAP
  	VOID GAP_SetParamValue(TGAP_CONN_PAUSE_PERIPHERAL, DEFAULT_CONN_PAUSE_PERIPHERAL);
  
  	// Setup the GAP Peripheral Role Profile
  	{
      	// For other hardware platforms, device starts advertising upon initialization
      	uint8 initial_advertising_enable = TRUE;

    	// By setting this to zero, the device will go into the waiting state after
    	// being discoverable for 30.72 second, and will not being advertising again
    	// until the enabler is set back to TRUE
    	uint16 gapRole_AdvertOffTime = 0;

    	uint8 enable_update_request = DEFAULT_ENABLE_UPDATE_REQUEST;
    	uint16 desired_min_interval = DEFAULT_DESIRED_MIN_CONN_INTERVAL;
		uint16 desired_max_interval = DEFAULT_DESIRED_MAX_CONN_INTERVAL;
    	uint16 desired_slave_latency = DEFAULT_DESIRED_SLAVE_LATENCY;
    	uint16 desired_conn_timeout = DEFAULT_DESIRED_CONN_TIMEOUT;

    	// Set the GAP Role Parameters
    	GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &initial_advertising_enable );
    	GAPRole_SetParameter( GAPROLE_ADVERT_OFF_TIME, sizeof( uint16 ), &gapRole_AdvertOffTime );

    	GAPRole_SetParameter( GAPROLE_SCAN_RSP_DATA, sizeof ( scanRspData ), scanRspData );
    	GAPRole_SetParameter( GAPROLE_ADVERT_DATA, sizeof( advertData ), advertData );

    	GAPRole_SetParameter( GAPROLE_PARAM_UPDATE_ENABLE, sizeof( uint8 ), &enable_update_request );
    	GAPRole_SetParameter( GAPROLE_MIN_CONN_INTERVAL, sizeof( uint16 ), &desired_min_interval );
    	GAPRole_SetParameter( GAPROLE_MAX_CONN_INTERVAL, sizeof( uint16 ), &desired_max_interval );
    	GAPRole_SetParameter( GAPROLE_SLAVE_LATENCY, sizeof( uint16 ), &desired_slave_latency );
    	GAPRole_SetParameter( GAPROLE_TIMEOUT_MULTIPLIER, sizeof( uint16 ), &desired_conn_timeout );
  	}

  	// Set the GAP Characteristics
  	GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, attDeviceName);

  	// Set advertising interval
  	{
    	uint16 advInt = DEFAULT_ADVERTISING_INTERVAL;

    	GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MIN, advInt);
    	GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MAX, advInt);
    	GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MIN, advInt);
    	GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MAX, advInt);
  	}

  	// Initialize GATT attributes
  	GGS_AddService(GATT_ALL_SERVICES);            // GAP
  	GATTServApp_AddService(GATT_ALL_SERVICES);    // GATT attributes

#if defined FEATURE_OAD    
	  VOID OADTarget_AddService();					  // OAD Profile	
#endif

	GascanProfile_AddService(GATT_ALL_SERVICES);  // gascan GATT Profile

	GascanProfile_RegisterAppCBs(&gascanProfileCBs);
	
  	// Enable clock divide on halt
  	// This reduces active current while radio is active and CC254x MCU
  	// is halted
  	HCI_EXT_ClkDivOnHaltCmd(HCI_EXT_DISABLE_CLK_DIVIDE_ON_HALT);

#if defined ( DC_DC_P0_7 )

  	// Enable stack to toggle bypass control on TPS62730 (DC/DC converter)
  	HCI_EXT_MapPmIoPortCmd(HCI_EXT_PM_IO_PORT_P0, HCI_EXT_PM_IO_PORT_PIN7);

#endif // defined ( DC_DC_P0_7 )

  	// Setup a delayed profile startup
  	osal_set_event(gascan_TaskID, GASCAN_START_DEVICE_EVT);
}

/*
static void TestMeasureCallback(uint16 kPa)
{
	TRACE("%dkPa\r\n", kPa);
}
*/

/*********************************************************************
 * @fn      Gascan_ProcessEvent
 *
 * @brief   PowerAsist Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16 Gascan_ProcessEvent(uint8 task_id, uint16 events)
{
	VOID task_id; // OSAL required parameter that isn't used in this function

	if (events & SYS_EVENT_MSG)
	{
		uint8 *pMsg;

		if ((pMsg = osal_msg_receive(gascan_TaskID)) != NULL)
		{
			Gascan_ProcessOSALMsg((osal_event_hdr_t *)pMsg);

			// Release the OSAL message
			osal_msg_deallocate(pMsg);
		}

		// return unprocessed events
		return events ^ SYS_EVENT_MSG;
	}

	//onInit
	if (events & GASCAN_START_DEVICE_EVT)
	{
		// Start the Device
		VOID GAPRole_StartDevice(&gascan_PeripheralCBs);
		
		osal_start_timerEx(gascan_TaskID, GASCAN_POWER_ON_DELAY_EVT, POWER_ON_DELAY_TIME);
		
		return events ^ GASCAN_START_DEVICE_EVT;
	}

	if (events & GASCAN_POWER_ON_DELAY_EVT)
	{
		uint16 vol = GetBatteryVoltage();
		if (vol <= LOW_POWER_THRESHOLD)
		{
			IndicateLowPower();
		}
		else
		{
			IndicateNormalPower();
		}
		
		return events ^ GASCAN_POWER_ON_DELAY_EVT;
	}
	
	if (events & GASCAN_CONNECTED_PEROID_EVT)
	{
		uint16 vol = GetBatteryVoltage();
		TRACE("bat vol:%d.%02dV\r\n", vol / 100, vol % 100);
		
		if (vol <= LOW_POWER_THRESHOLD)
		{
			IndicateConnectedInLowPower();
		}
		else
		{
			IndicateConnectedInNormalPower();
		}
		
		return events ^ GASCAN_CONNECTED_PEROID_EVT;
	}

	if (events & GASCAN_PARSE_PACKET_TIMEOUT_EVT)
	{
		//parse packet timeout
		ResetParsePacket();

		TRACE("parse packet timeout\r\n");
		
		return events ^ GASCAN_PARSE_PACKET_TIMEOUT_EVT;
	}
	
	if (events & GASCAN_UPDATE_SCAN_RSP_DATA_EVT)
	{
		//update scan response data
		GAP_UpdateAdvertisingData(gascan_TaskID,     
                             FALSE,    
                             sizeof(scanRspData),    
                             scanRspData);

		if (GetNeedUpdateParam())
		{
			SaveParameter();
		}
		
        //re-enable ad
        //uint8 initial_advertising_enable = TRUE;    
		//GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &initial_advertising_enable);
 
		return events ^ GASCAN_UPDATE_SCAN_RSP_DATA_EVT;
	}
	
#if defined ( PLUS_BROADCASTER )
	if (events & POWERASIST_ADV_IN_CONNECTION_EVT)
	{
		uint8 turnOnAdv = TRUE;
		// Turn on advertising while in a connection
		GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &turnOnAdv);

		return (events ^ POWERASIST_ADV_IN_CONNECTION_EVT);
	}
#endif // PLUS_BROADCASTER

  // Discard unknown events
  return 0;
}

void StartGascanTimer(uint16 event, uint32 timeout, bool repeat)
{
	if (repeat)
	{
		osal_start_reload_timer(gascan_TaskID, event, timeout);
	}
	else
	{
		osal_start_timerEx(gascan_TaskID, event, timeout);
	}
}

void StopGascanTimer(uint16 event)
{
	osal_stop_timerEx(gascan_TaskID, event);
}

void UpdateBleName(const uint8 name[BLE_NAME_LEN])
{
	SetBleName(name);

	GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scanRspData), scanRspData);	
  	GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, attDeviceName);
}

/*********************************************************************
 * @fn      Gascan_ProcessOSALMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void Gascan_ProcessOSALMsg( osal_event_hdr_t *pMsg )
{
	switch (pMsg->event)
	{
    case KEY_CHANGE:
    	{

    	}
    	
		break;

	default:
		// do nothing
		break;
	}
}

/*********************************************************************
 * @fn      peripheralStateNotificationCB
 *
 * @brief   Notification from the profile of a state change.
 *
 * @param   newState - new state
 *
 * @return  none
 */
static void peripheralStateNotificationCB(gaprole_States_t newState)
{
	switch ( newState )
	{
	case GAPROLE_STARTED:
		{
			TRACE("started..\r\n");

			//NPI_Print("started..\r\n");
		}
		break;

	case GAPROLE_ADVERTISING:  //ÕýÔÚ¹ã²¥
		{
			TRACE("advertising\r\n");

			//NPI_Print("advertising\r\n");
		}
		break;

	case GAPROLE_CONNECTED:  //cnnOK
		{
			TRACE("connected\r\n");

			//NPI_Print("connected\r\n");
			
			uint16 vol = GetBatteryVoltage();
			if (vol <= LOW_POWER_THRESHOLD)
			{
				IndicateConnectedInLowPower();
			}
			else
			{
				IndicateConnectedInNormalPower();
			}
			
			osal_start_reload_timer(gascan_TaskID, GASCAN_CONNECTED_PEROID_EVT, DETECT_INTERVAL_WHEN_CONNECTED);

			SetNeedUpdateParam(false);
		}
		break;

	case GAPROLE_WAITING:
		{
			TRACE("disconnected\r\n");

			//NPI_Print("disconnected\r\n");
			
			BleDisconnected();
		}

		break;

	case GAPROLE_WAITING_AFTER_TIMEOUT:
		{
			TRACE("timeout.\r\n");

			//NPI_Print("timeout.\r\n");
			
			BleDisconnected();
		}

		break;

	case GAPROLE_ERROR:
		{
			TRACE("err.\r\n");

			//NPI_Print("err.\r\n");
			
			BleDisconnected();
		}

		break;

	default:
		{
		}

		break;
	}

	gapProfileState = newState;

	VOID gapProfileState;     // added to prevent compiler warning with
	            				// "CC2540 Slave" configurations
}

static void GascanProfileChangeCB(uint8 paramID, uint8 len)
{
	uint8 buf[20];

	switch (paramID)
	{
    case GASCANPROFILE_TRX:
    	{
			bStatus_t status = GascanProfile_GetParameter(GASCANPROFILE_TRX, buf, len);
			if (status == SUCCESS)
			{
				ProcessBleCom(buf, len);
			}
		}
		
      	break;

    default:
      	/* should not reach here! */

		break;
	}
}

