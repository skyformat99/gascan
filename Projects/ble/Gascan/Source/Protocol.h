
#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

//max packet len 
#define MAX_PACKET_LEN              40

//ble name length
#define BLE_NAME_LEN                20

//header 
#define PACKET_HEADER               0xA5

//protocol version
#define PROTOCOL_VERSION            1

//data type
#define TYPE_MEASURE                1
#define TYPE_START_CALIBRATION      2
#define TYPE_TEMP_CALIBRATE         3
#define TYPE_PRESSURE_CALIBRATE     4

#define TYPE_QUERY_PARAM            5
#define TYPE_SET_BLE_NAME           6

#define TYPE_TEST                   21

//data bitmap
#define DATA_TEMPERATURE_POS        0
#define DATA_TEMPERATURE_MASK       (1 << 0)
#define DATA_PRESSURE_POS           1
#define DATA_PRESSURE_MASK          (1 << 1)
#define DATA_VOLTAGE_POS            2
#define DATA_VOLTAGE_MASK           (1 << 2)

//packet data len
#define SIZE_GET_MEASURE_DATA       2
#define SIZE_START_CALIBRATION      3
#define SIZE_TEMP_CALIBRATE         3
#define SIZE_PRESSURE_CALIBRATE     3
#define SIZE_SET_BLE_NAME           21

//calibration action
#define CALIBRATION_START           1
#define CALIBRATION_STOP            0

//calibration type
#define CALIBRATION_TYPE_TEMP       1
#define CALIBRATION_TYPE_PRESSURE   2

#define RESULT_OK                   0
#define RESULT_FAILED               1

//reason
#define REASON_NONE                       0
#define REASON_TEMP_NOT_CALIBRATED        1
#define REASON_TEMP_CALIBRATE_STARTED     2
#define REASON_TEMP_CALIBRATE_NOT_STARTED 3
#define REASON_PRESSURE_NOT_CALIBRATED    4
#define REASON_PRESSURE_CALIBRATE_STARTED 5
#define REASON_PRESSURE_CALIBRATE_NOT_STARTED 6
#define REASON_PRESSURE_CALIBRATION_BEING_DONE 7
#define REASON_PRESSURE_CALIBRATION_POINT_NUM_WRONG  8

#define REASON_OTHER                      0xff

#endif

