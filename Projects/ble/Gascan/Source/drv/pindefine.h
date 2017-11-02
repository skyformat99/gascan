
#ifndef _PINDEFINE_H_
#define _PINDEFINE_H_

#if defined( CC2541) || defined( CC2541S )
#include <ioCC2541.h>
#else // CC2540
#include <ioCC2540.h>
#endif // CC2541 || CC2541S


#include "gpio.h"

/* i2c pin */
#define PORT_PIN_SDA     1
#define OFFSET_PIN_SDA   3
#define DIR_PIN_SDA      PORT_DIR(PORT_PIN_SDA)
#define PIN_SDA          MAKE_GPIO(PORT_PIN_SDA, OFFSET_PIN_SDA)


#define PORT_PIN_SCL     1
#define OFFSET_PIN_SCL   2
#define DIR_PIN_SCL      PORT_DIR(PORT_PIN_SCL)
#define PIN_SCL          MAKE_GPIO(PORT_PIN_SCL, OFFSET_PIN_SCL)

//pin clk
#define PORT_PIN_SLK     0
#define OFFSET_PIN_SLK   5
#define DIR_PIN_SLK      PORT_DIR(PORT_PIN_SLK)
#define PIN_SLK          MAKE_GPIO(PORT_PIN_SLK, OFFSET_PIN_SLK)

//pin dt
#define PORT_PIN_DT      0
#define OFFSET_PIN_DT    6
#define DIR_PIN_DT       PORT_DIR(PORT_PIN_DT)
#define PIN_DT           MAKE_GPIO(PORT_PIN_DT, OFFSET_PIN_DT)

#if 0
//pin led
#define PORT_PIN_LED_R     1
#define OFFSET_PIN_LED_R   1
#define DIR_PIN_LED_R      PORT_DIR(PIN_LED_R)
#define PIN_LED_R          MAKE_GPIO(PORT_PIN_LED_R, OFFSET_PIN_LED_R)

#define PORT_PIN_LED_G     1
#define OFFSET_PIN_LED_G   0
#define DIR_PIN_LED_G      PORT_DIR(PIN_LED_G)
#define PIN_LED_G          MAKE_GPIO(PORT_PIN_LED_G, OFFSET_PIN_LED_G)

#endif

#endif

