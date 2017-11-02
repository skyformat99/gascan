/**************************************************************************************************
  Filename:       gascanGATTprofile.c
  Revised:        $Date: 2013-05-06 13:33:47 -0700 (Mon, 06 May 2013) $
  Revision:       $Revision: 34153 $

  Description:    This file contains the Gascan GATT profile sample GATT service 
                  profile for use with the BLE sample application.

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
#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "gapbondmgr.h"

#include "npi.h"
#include "gascanGATTprofile.h"

/*********************************************************************
 * MACROS
 */
#define   BALANCEPROFILE_CHAR_TRX_VAL_INDEX      2

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// Gascan Profile Service UUID: 0xFFF0
static CONST uint8 gascanProfileServUUID[ATT_BT_UUID_SIZE] =
{ 
	LO_UINT16(GASCANPROFILE_SERV_UUID), HI_UINT16(GASCANPROFILE_SERV_UUID)
};

// Characteristic trx UUID: 0xFFF1
static CONST uint8 gascanTRxUUID[ATT_BT_UUID_SIZE] =
{ 
	LO_UINT16(GASCAN_TRX_UUID), HI_UINT16(GASCAN_TRX_UUID)
};

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

static gascanProfileCBs_t *gascanProfile_AppCBs = NULL;

/*********************************************************************
 * Profile Attributes - variables
 */

// Gascan Profile Service attribute
static CONST gattAttrType_t gascanProfileService = { ATT_BT_UUID_SIZE, gascanProfileServUUID };

// Gascan Profile Characteristic TRx Properties
static uint8 gascanProfileTRxProps = GATT_PROP_WRITE_NO_RSP | GATT_PROP_NOTIFY;
// Characteristic TRx Value
static uint8 gascanProfileTRx[20] = { 0 };

// Characteristic TRx Config
static gattCharCfg_t gascanProfileTRxConfig[GATT_MAX_NUM_CONN];

// Characteristic TRx User Description
static uint8 gascanProfileTRxUserDesp[] = "Tx&Rx";
/*********************************************************************
 * Profile Attributes - Table
 */
static gattAttribute_t gascanProfileAttrTbl[] = 
{
	// Gascan Profile Service
	{ 
		{ ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
		GATT_PERMIT_READ,						  /* permissions */
		0,										  /* handle */
		(uint8 *)&gascanProfileService		      /* pValue */
	},

	// Characteristic TRx Declaration
	{ 
		{ ATT_BT_UUID_SIZE, characterUUID },
		GATT_PERMIT_READ, 
		0,
		&gascanProfileTRxProps 
	},

	// Characteristic TRx Value
	{ 
		{ ATT_BT_UUID_SIZE, gascanTRxUUID },
		GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
		0, 
		gascanProfileTRx
	},

	// Characteristic TRx configuration
	{ 
		{ ATT_BT_UUID_SIZE, clientCharCfgUUID },
		GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
		0, 
		(uint8 *)gascanProfileTRxConfig 
	},
	  
	// Characteristic TRx User Description
	{ 
		{ ATT_BT_UUID_SIZE, charUserDescUUID },
		GATT_PERMIT_READ, 
		0, 
		gascanProfileTRxUserDesp 
	},
};


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static bStatus_t gascanProfile_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );

static void gascanProfile_HandleConnStatusCB( uint16 connHandle, uint8 changeType );


/*********************************************************************
 * PROFILE CALLBACKS
 */
// Gascan Profile Service Callbacks
CONST gattServiceCBs_t gascanProfileCBs =
{
	NULL,  // Read callback function pointer
	gascanProfile_WriteAttrCB, // Write callback function pointer
	NULL                       // Authorization callback function pointer
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      GascanProfile_AddService
 *
 * @brief   Initializes the Gascan Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t GascanProfile_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, gascanProfileTRxConfig );
	
  // Register with Link DB to receive link status change callback
  VOID linkDB_Register(gascanProfile_HandleConnStatusCB );  
  
  if (services & GASCANPROFILE_SERVICE)
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService(gascanProfileAttrTbl, 
                                          GATT_NUM_ATTRS( gascanProfileAttrTbl ),
                                          &gascanProfileCBs );
  }

  return ( status );
}


/*********************************************************************
 * @fn      GascanProfile_RegisterAppCBs
 *
 * @brief   Registers the application callback function. Only call 
 *          this function once.
 *
 * @param   callbacks - pointer to application callbacks.
 *
 * @return  SUCCESS or bleAlreadyInRequestedMode
 */
bStatus_t GascanProfile_RegisterAppCBs(gascanProfileCBs_t *appCallbacks )
{
  if ( appCallbacks )
  {
    gascanProfile_AppCBs = appCallbacks;
    
    return ( SUCCESS );
  }
  else
  {
    return ( bleAlreadyInRequestedMode );
  }
}
 
/*********************************************************************
 * @fn      GascanProfile_GetParameter
 *
 * @brief   Get a Gascan Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to put.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t GascanProfile_GetParameter( uint8 param, void *value, uint8 len)
{
	bStatus_t ret = SUCCESS;
	switch ( param )
	{
    case GASCANPROFILE_TRX:
		osal_memcpy(value, gascanProfileTRx, len);
      
		break;
	  
    default:
		ret = INVALIDPARAMETER;
		break;
	}
  
	return ( ret );

}

/*********************************************************************
 * @fn      gascanProfile_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 *
 * @return  Success or Failure
 */
static bStatus_t gascanProfile_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset )
{
	bStatus_t status = SUCCESS;
	uint8 notifyApp = 0xFF;
  
	// If attribute permissions require authorization to write, return error
	if ( gattPermitAuthorWrite( pAttr->permissions ) )
	{
		// Insufficient authorization
		return ( ATT_ERR_INSUFFICIENT_AUTHOR );
	}
  
	if ( pAttr->type.len == ATT_BT_UUID_SIZE )
	{
    	// 16-bit UUID
    	uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
	    if (uuid == GASCAN_TRX_UUID)
	    {
			if (status == SUCCESS)
			{
				uint8 *pCurValue = (uint8 *)pAttr->pValue;
				osal_memcpy(pCurValue, pValue, len);

				if (pAttr->pValue == gascanProfileTRx)
				{
					notifyApp = GASCANPROFILE_TRX;
				}
			}
	    }
	    else if (uuid == GATT_CLIENT_CHAR_CFG_UUID)
	    {
			status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,    
			                                             offset, GATT_CLIENT_CFG_NOTIFY ); 
	    }
	    else
	    {
			status = ATT_ERR_ATTR_NOT_FOUND;
	    }
	}
	else
	{
		// 128-bit UUID
    	status = ATT_ERR_INVALID_HANDLE;
	}

    // If a charactersitic value changed then callback function to notify application of change
	if ( (notifyApp != 0xFF ) && gascanProfile_AppCBs && gascanProfile_AppCBs->pfnGascanProfileChange )
	{
		gascanProfile_AppCBs->pfnGascanProfileChange(notifyApp, len);  
	}
  
	return ( status );

}

/*********************************************************************
 * @fn          gascanProfile_HandleConnStatusCB
 *
 * @brief       Gascan Profile link status change handler function.
 *
 * @param       connHandle - connection handle
 * @param       changeType - type of change
 *
 * @return      none
 */
static void gascanProfile_HandleConnStatusCB( uint16 connHandle, uint8 changeType )
{ 
  // Make sure this is not loopback connection
  if ( connHandle != LOOPBACK_CONNHANDLE )
  {
    // Reset Client Char Config if connection has dropped
    if ( ( changeType == LINKDB_STATUS_UPDATE_REMOVED )      ||
         ( ( changeType == LINKDB_STATUS_UPDATE_STATEFLAGS ) && 
           ( !linkDB_Up( connHandle ) ) ) )
    { 
      GATTServApp_InitCharCfg( connHandle, gascanProfileTRxConfig );
    }
  }
}

uint8 GascanProfile_Notify(const uint8 *pbuf, uint8 length)
{
	uint8 len;
	
	attHandleValueNoti_t pReport;
	
	if (length > 20)
	{
		len = 20;
	}
	else
	{
		len = length;
	}
	
	pReport.handle = gascanProfileAttrTbl[BALANCEPROFILE_CHAR_TRX_VAL_INDEX].handle;
	
	pReport.len = len;
	
	osal_memcpy(pReport.value, pbuf, len);
	
	GATT_Notification(0, &pReport, FALSE);

	return len;
}

/*********************************************************************
*********************************************************************/
