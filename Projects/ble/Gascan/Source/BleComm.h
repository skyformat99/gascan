
#ifndef _BLECOMM_H_
#define _BLECOMM_H_

extern void SetNeedUpdateParam(bool need);
extern bool GetNeedUpdateParam();

extern void BleDisconnected();

extern void ProcessBleCom(const uint8 *buf, uint8 len);

#endif
