
#ifndef _HX711_H_
#define _HX711_H_

typedef enum
{
	Channel_A_128 = 0,

	Channel_B_32,

	Channel_A_64,
} Hx711Gain;

extern void InitHx711();

extern void StartHx711(void (*callback)(uint32), Hx711Gain gain, bool repeat);

extern void StopHx711();

extern void EnterHx711SleepMode();

extern void ProcessHx711Event();

#endif

