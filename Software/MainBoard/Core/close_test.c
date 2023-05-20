#include <string.h>

#include "CMS32L051.h"
#include "userdefine.h"
#include "gpio.h"
#include "intp.h"
#include "device.h"
#include "log.h"
#include "safe_box.h"
#include "sci.h"
void CloseTestInit(void);
void CloseTestSleep(void);

struct Device closeTest = { NULL, CloseTestInit, CloseTestSleep };
static MultiTimer closeTestTimer,mainSleepTimer;
void CloseTestCallback(MultiTimer* timer, void* userData);
extern void UART2_SendData(uint8_t data[]);
void CloseTestInit()
{
    PORT_Init(PORT2, PIN3, PULLUP_INPUT);
    MultiTimerStart(&closeTestTimer, 10, CloseTestCallback, NULL);
}
void CloseTestCallback(MultiTimer* timer, void* userData)
{
    uint8_t open[] = {DOOR_OPEN};
    uint8_t close[] = {DOOR_CLOSE};
    static uint8_t lastaState = 1;
    uint8_t state = !PORT_GetBit(PORT2, PIN3);
    if (state != lastaState) {
        if (!state) {
            // SafeBoxFsm(DOOR_OPEN, NULL);
            //UART_SendData(open,1);
            UART2_SendData(open);
        } else {
            // SafeBoxFsm(DOOR_CLOSE, NULL);
            //UART_SendData(close,1);
			UART2_SendData(close);
        }
        lastaState = state;
    }
    if (!state) {
    }
    MultiTimerStart(&closeTestTimer, 10, CloseTestCallback, NULL);
}

void CloseTestSleep()
{
    INTP_Stop(1 << 1);
    INTP_Init(1 << 1, INTP_RISING);
    INTP_Start(1 << 1);
}
