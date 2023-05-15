#include "device.h"
#include "CMS32L051.h"
#include "userdefine.h"
#include "gpio.h"
#include <string.h>
#include "key_scan.h"

void KeyIoInit(void);
void KeyScan(MultiTimer* timer, void* userData);
void AsleepKeyBoard(MultiTimer* timer, void* userData);
void KeyEventHandler(int keyValue, enum KeyEvent event);
void KeyEventCallback(int keyValue, enum KeyEvent event);
MultiTimer keyTimer, sleepTimer;
struct Device keyBoard = { DEVICE_RUN, KeyIoInit };
#define KEY_CNT 2
struct Gpio {
    PORT_TypeDef port;
    PIN_TypeDef pin;
};
const struct Gpio keyIO[] = {
    { PORT1, PIN0 },
    { PORT2, PIN1 },
};
const uint8_t keyChnList[] = { 0,1 };
enum KeyState keyStateList[] = { KEY_CHECK, KEY_CHECK };
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
}
void KeyScan(MultiTimer* timer, void* userData)
{
    int i = 0;
    for (i = 0; i < KEY_CNT; ++i) {
       KeyFilter(IfPush(i), &keyStateList[i], i+12, KeyEventCallback);
    }

    MultiTimerStart(&keyTimer, SCAN_PERIOD, KeyScan, NULL);
}
void KeyEventCallback(int keyValue, enum KeyEvent event)
{
    KeyEventHandler(keyValue, event);
}
void AsleepKeyBoard(MultiTimer* timer, void* userData)
{
    keyBoard.state = DEVICE_SLEEP;
    // 打开外部中断，
}
