#include "comdef.h"
#include "hal_types.h"

#include "hal_drivers.h"
#include "osal.h"
#include "hal_mcu.h"

#include "PinDefine.h"

#include "HX711.h"

#include "npi.h"

#define HX711_INT_RISING_EDGE   0
#define HX711_INT_FALLING_EDGE  1

/* CPU port interrupt */
#define HX711_INT_CPU_PORT_0_IF  P0IF

/* int is at P0.6 */
#define HX711_INT_PORT   P0
#define HX711_INT_BIT    BV(6)
#define HX711_INT_SEL    P0SEL
#define HX711_INT_DIR    P0DIR

/* edge interrupt */
#define HX711_INT_EDGEBIT  BV(0)
#define HX711_INT_EDGE     HX711_INT_FALLING_EDGE

/* interrupts */
#define HX711_INT_IEN      IEN1  /* CPU interrupt mask register */
#define HX711_INT_IENBIT   BV(5) /* Mask bit for all of Port_0 */
#define HX711_INT_ICTL     P0IEN /* Port Interrupt Control register */
#define HX711_INT_ICTLBIT  BV(6) /* P0IEN - P0.6 enable/disable bit */
#define HX711_INT_PXIFG    P0IFG /* Interrupt flag at source */

/* mode */
#define HX711_PORT_MODE    P0INP
#define HX711_PORT_MODEBIT BV(6)

#define SLEEP_TIMEOUT        10000ul

//callback
static void (*s_hx711callback)(uint32) = NULL;

static bool s_repeat = false;

#define TRUE_ADC_COUNT     5

static uint8 s_trueAdcCount = 0;

static Hx711Gain s_gain = Channel_A_128;

static uint8 s_skipClkCount = 1;

static bool s_bInSleep = true;

static void EnableDoutInterrupt(bool enable)
{
	HX711_INT_PXIFG &= ~(HX711_INT_BIT);
	
	if (enable)
	{
		HX711_INT_ICTL |= HX711_INT_ICTLBIT;
	}
	else
	{
		HX711_INT_ICTL &= ~(HX711_INT_ICTLBIT);
	}
}

void InitHx711()
{
	SET_GPIO_OUTPUT(PIN_SLK);
	SET_GPIO_BIT(PIN_SLK);
	
	HX711_INT_SEL &= ~(HX711_INT_BIT);    /* Set pin function to GPIO */
  	HX711_INT_DIR &= ~(HX711_INT_BIT);    /* Set pin direction to Input */

	//tristate mode
	HX711_PORT_MODE |= HX711_PORT_MODEBIT;
	
  	/* Rising/Falling edge configuratinn */
    PICTL &= ~(HX711_INT_EDGEBIT);    /* Clear the edge bit */
    /* For falling edge, the bit must be set. */
  #if (HX711_INT_EDGE == HX711_INT_FALLING_EDGE)
    PICTL |= HX711_INT_EDGEBIT;
  #endif

	HX711_INT_IEN |= HX711_INT_IENBIT;
}

static uint32 GetHx711ADCValue()
{
	uint32 adc = 0;
	uint8 i;

	halIntState_t intState;
	HAL_ENTER_CRITICAL_SECTION(intState);
	
	for (i = 0; i < 24; i++)
	{
		SET_GPIO_BIT(PIN_SLK);

		adc <<= 1;
		
		CLR_GPIO_BIT(PIN_SLK);

		if (GET_GPIO_BIT(PIN_DT))
		{
			adc |= 0x01;
		}
	}

	adc ^= 0x800000;

	for (i = 0; i < s_skipClkCount; i++)
	{
		SET_GPIO_BIT(PIN_SLK);
		asm("NOP");
		asm("NOP");
		asm("NOP");
		CLR_GPIO_BIT(PIN_SLK);
	}

	HAL_EXIT_CRITICAL_SECTION(intState);
	
	return adc;
}

HAL_ISR_FUNCTION(intPort0Isr, P0INT_VECTOR)
{
	HAL_ENTER_ISR();

	if (HX711_INT_PXIFG & HX711_INT_BIT)
	{
		EnableDoutInterrupt(false);

		osal_set_event(Hal_TaskID, HX711_DOUT_EVENT);
	}

	/*
		Clear the CPU interrupt flag for Port_1
		PxIFG has to be cleared before PxIF
  	*/
  
	HX711_INT_CPU_PORT_0_IF = 0;

	CLEAR_SLEEP_MODE();

	HAL_EXIT_ISR();
}

void StartHx711(void (*callback)(uint32), Hx711Gain gain, bool repeat)
{
	TRACE("StartHx711\r\n");
	
	s_hx711callback = callback;
	s_repeat = repeat;
	
	if (s_bInSleep)
	{
		if (gain == Channel_A_128)
		{
			s_trueAdcCount = 0;
		}
		else
		{
			s_trueAdcCount = TRUE_ADC_COUNT;
		}
	}
	else
	{
		if (s_gain == gain)
		{
			s_trueAdcCount = 0;
		}
		else
		{
			s_trueAdcCount = TRUE_ADC_COUNT;
		}
	}

	s_gain = gain;

	if (s_gain == Channel_A_128)
	{
		s_skipClkCount = 1;
	}
	else if (s_gain == Channel_B_32)
	{
		s_skipClkCount = 2;
	}
	else if (s_gain == Channel_A_64)
	{
		s_skipClkCount = 3;
	}
	
	CLR_GPIO_BIT(PIN_SLK);
	
	EnableDoutInterrupt(true);

	s_bInSleep = false;

	//for timeout
	osal_start_timerEx(Hal_TaskID, HX711_SLEEP_EVENT, SLEEP_TIMEOUT);
}

void StopHx711()
{
	s_hx711callback = NULL;
	s_repeat = false;
	
	EnableDoutInterrupt(false);
}

void EnterHx711SleepMode()
{
	TRACE("hx711 enter sleep\r\n");
	
	SET_GPIO_BIT(PIN_SLK);
	
	s_bInSleep = true;
}

void ProcessHx711Event()
{
	uint32 adc = GetHx711ADCValue();

	//TRACE("adc count:%d,skip:%d\r\n", s_trueAdcCount, s_skipClkCount);
	TRACE("adc:0x%08lX\r\n", adc);
	
	if (s_trueAdcCount > 0 || s_repeat)
	{
		EnableDoutInterrupt(true);
	}

	if (s_trueAdcCount > 0)
	{
		s_trueAdcCount--;
	}
	else
	{
		if (s_hx711callback != NULL)
		{
			s_hx711callback(adc);
		}
	}
}

