#include "device.h"
#include "CMS32L051.h"
#include "userdefine.h"
#include "gpio.h"
#include "intp.h"
#include <string.h>
#include "key_filter.h"
#include "log.h"
//device def
void KeyIoInit(void);
void KeyBoardSleep(void);
struct Device keyBoard = { NULL, KeyIoInit, KeyBoardSleep };

//tiemr def
static MultiTimer keyTimer;
void KeyScan(MultiTimer* timer, void* userData);

//inner event handler
void KeyEventCallback(int keyValue, enum KeyEvent event);

//outer event handler
void ReportKeyEvent(int keyValue, enum KeyEvent event);


#define KEY_CNT 12
struct Gpio {
    PORT_TypeDef port;
    PIN_TypeDef pin;
};
const struct Gpio keyIO[] = {
    { PORT13, PIN6 },
    { PORT12, PIN3 },
    { PORT12, PIN4 },
    { PORT1, PIN4 },
    { PORT1, PIN3 },
    { PORT1, PIN2 },
    { PORT1, PIN1 },
    { PORT1, PIN0 },
    { PORT2, PIN3 },
    { PORT2, PIN2 },
    { PORT2, PIN1 },
    { PORT2, PIN0 },
};
const uint8_t keyChnList[] = { 4, 8, 0, 9, 1, 10, 2, 11, 3, 7, 6, 5 };
enum KeyState keyStateList[KEY_CNT];
#define IfPush(keyValue) (!PORT_GetBit(keyIO[keyChnList[keyValue]].port, keyIO[keyChnList[keyValue]].pin))
#define SCAN_PERIOD 10
void KeyIoInit()
{
    int i = 0;
    for (i = 0; i < KEY_CNT; ++i) {
        PORT_Init(keyIO[i].port, keyIO[i].pin, PULLUP_INPUT);
        keyStateList[i] = KEY_CHECK;
    }
    MultiTimerStart(&keyTimer, 0, KeyScan, NULL);
    MultiTimerStart(&deviceMgr.timer, SLEEP_TIME, SleepTimerCallBack, NULL);
}
void KeyBoardSleep()
{
    int i = 0;
    for (i = 0; i < KEY_CNT; ++i) {
        PORT_Init(keyIO[i].port, keyIO[i].pin, OUTPUT);
        PORT_ClrBit(keyIO[i].port, keyIO[i].pin);
    }
    INTP_Init(1 << 0, INTP_FALLING);
    INTP_Start(1 << 0);
    INTP_Init(1 << 1, INTP_FALLING);
    INTP_Start(1 << 1);
}

void KeyScan(MultiTimer* timer, void* userData)
{
    int i = 0;
    for (i = 0; i < KEY_CNT; ++i) {
        KeyFilter(IfPush(i), &keyStateList[i], i, KeyEventCallback);
    }

    MultiTimerStart(&keyTimer, SCAN_PERIOD, KeyScan, NULL);
}
void KeyEventCallback(int keyValue, enum KeyEvent event)
{
    PRINT("KeyValue = %d,event = %d\n",keyValue,event);
    // ReportKeyEvent(keyValue, event);
    MultiTimerStart(&deviceMgr.timer, SLEEP_TIME, SleepTimerCallBack, NULL);
}
