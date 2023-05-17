#include <string.h>
#include "CMS32L051.h"
#include "userdefine.h"
#include "gpio.h"
#include "device.h"
void LedInit(void);
void LedSleep(void);
MultiTimer ledTimer;
#define LED_CNT 4
uint8_t ledState[LED_CNT] = {0, 0, 0, 0};
struct Gpio
{
    PORT_TypeDef port;
    PIN_TypeDef pin;
};
const struct Gpio ledIO[] = {
    {PORT13, PIN7},
    {PORT2, PIN0},
    {PORT4, PIN0}};
const uint8_t ledIoNumList[][2] = {
    {2, 0},
    {1, 2},
    {0, 2},
    {0, 1}};

void LedTask(MultiTimer *timer, void *userData);
struct Device led = {NULL, LedInit, LedSleep};
#define LED_LOGO_1_H() PORT_SetBit(PORT13, PIN7)
#define LED_LOGO_1_L() PORT_ClrBit(PORT13, PIN7)
#define LED_LOGO_2_H() PORT_SetBit(PORT2, PIN0)
#define LED_LOGO_2_L() PORT_ClrBit(PORT2, PIN0)
#define LED_LOGO_3_H() PORT_SetBit(PORT4, PIN0)
#define LED_LOGO_3_L() PORT_ClrBit(PORT4, PIN0)
#define CLOSE_ALL_LED()                 \
    {                                   \
        PORT_Init(PORT13, PIN7, INPUT); \
        PORT_Init(PORT2, PIN0, INPUT);  \
        PORT_Init(PORT4, PIN0, INPUT);  \
    }
void LedInit()
{
    int i = 0;
    PORT_Init(PORT12, PIN4, OUTPUT);
    CLOSE_ALL_LED();
    PRINT("CLOSE_ALL_LED\n");
    for (i = 0; i < LED_CNT; ++i)
    {
        ledState[i] = 0;
    }
    MultiTimerStart(&ledTimer, 1000, LedTask, NULL);
}

void LedTask(MultiTimer *timer, void *userData)
{
    static int8_t ledNum = 0;
    CLOSE_ALL_LED();
    if (ledState[ledNum])
    {
        PORT_Init(ledIO[ledIoNumList[ledNum][1]].port, ledIO[ledIoNumList[ledNum][1]].pin, OUTPUT);
		PORT_Init(ledIO[ledIoNumList[ledNum][0]].port, ledIO[ledIoNumList[ledNum][0]].pin, OUTPUT);
        PORT_ClrBit(ledIO[ledIoNumList[ledNum][1]].port, ledIO[ledIoNumList[ledNum][1]].pin);
        
        PORT_SetBit(ledIO[ledIoNumList[ledNum][0]].port, ledIO[ledIoNumList[ledNum][0]].pin);
    }
     ledNum++;
     if (ledNum >= 4) {
         ledNum = 0;
     }
    // PORT_SetBit(PORT12, PIN4);
    /* PORT_Init(PORT13, PIN7, OUTPUT);
    PORT_Init(PORT4, PIN0, OUTPUT);
    PORT_Init(PORT2, PIN0, INPUT);
    PORT_SetBit(PORT4, PIN0);
    PORT_ClrBit(PORT13, PIN7); */
    MultiTimerStart(&ledTimer, 5, LedTask, NULL);
}
void SetLedState(uint8_t Num, uint8_t state)
{
    if (Num < LED_CNT)
    {
        //    ledState[Num] = state;
    }
    else
    {
        if (state)
        {
            PORT_SetBit(PORT12, PIN4);
            PRINT("keyBoard open!");
        }
        else
        {
            PORT_ClrBit(PORT12, PIN4);
            PRINT("keyBoard close!");
        }
    }
}
void LedSleep()
{
    CLOSE_ALL_LED();
    PORT_ClrBit(PORT12, PIN4);
    PRINT("led sleep\n");
}
