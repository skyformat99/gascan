
#ifndef _TMP75_H_
#define _TMP75_H_

extern bool InitTMP75();

extern bool StartTMP75Conversion();

extern bool StopTMP75Conversion();

extern bool GetTMP75Temperature(uint16 *tempValue);

#endif

