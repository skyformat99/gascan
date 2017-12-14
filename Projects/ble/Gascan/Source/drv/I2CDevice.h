
#ifndef _I2CDEVICE_H_
#define _I2CDEVICE_H_

extern bool DetectI2CDevice(uint8 dev);

extern bool WriteI2CDeviceData(uint8 dev, const uint8 *addr, uint8 addrLen, const uint8 *pData, uint16 len);

extern bool ReadI2CDeviceData(uint8 dev, const uint8 *addr, uint8 addrLen, uint8 *pData, uint16 len, uint16 *pRetLen);

#endif

