#include <string.h>

#include "CMS32L051.h"
#include "userdefine.h"
#include "gpio.h"
#include "intp.h"

#include "device.h"
#include "key_filter.h"
#include "log.h"

//#include "safe_box.h"
#include "sci.h"

enum {
    READ_USER_LIST_FINISH,
    DELAY_START_TIME_ENEUGH,
    READ_USER_LIST_TIMEOUT,

    IDENTIFY_SUCCESS,
    IDENTIFY_FAIL,
    REG_ONCE_SUCCESS,
    REG_ONCE_FAIL,
    REG_ONE_FINGER_SUCCESS,
    REG_ONE_FINGER_FAIL,
    MODE_SWITCH,
    VIBRATION_EVENT,
    ALARM_TIME_ENOUGH,
    REGISTER_CMD,
    INPUT_PASSWORD_TIMEOUT,
    EXIT_REGISTER,
    ALTERNATE_SYSTEM_UNLOCK,
    RESTORE_FACTORY_CMD,
    PASSWORD_BACKUP_SUCCESS,
    RESTORE_FACTORY_FAIL,
    DOOR_OPEN,
    DOOR_CLOSE,
    LOW_BATTERY_ALARM,
    MOTOR_TASK_FINISH,
    // DOOR_OPEN_TIMEOUT
};

#define KEY_SET          0
#define KEY_MODE         1
#define LONG_TRIGGER     2

//device def
void KeyIoInit(void);
void KeyBoardSleep(void);
struct Device keyBoard = { NULL, KeyIoInit, KeyBoardSleep };

//tiemr def
static MultiTimer keyTimer;
void KeyScan(MultiTimer* timer, void* userData);

/* test */
MultiTimer setKeyLongPushTimer,modeKeyLongPushTimer;
void KeyEventCallback(int keyValue, enum KeyEvent event);
void (*keyEventHandler)(int value, uint8_t eventType) = NULL;
void LongPushDetect(MultiTimer* timer, void* userData);
void TestOpenDoorHandler(int keyValue, uint8_t event);
extern void UART2_SendData(uint8_t* sendData);
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
    MultiTimerStart(&keyTimer, 1000, KeyScan, NULL);
}
void KeyBoardSleep()
{
    int i = 0;
    for (i = 0; i < KEY_CNT; ++i) {
        PORT_Init(keyIO[i].port, keyIO[i].pin, OUTPUT);
        PORT_ClrBit(keyIO[i].port, keyIO[i].pin);
    }
    
}
void KeyScan(MultiTimer* timer, void* userData)
{
    int i = 0;
    for (i = 0; i < KEY_CNT; ++i) {
			//PRINT("i=%d,push=%d\n",i,IfPush(i));
        // PRINT("key:%d,state:%d\n",i+12,IfPush(i));
    //    KeyFilter(IfPush(i), &keyStateList[i], i+12,keyEventHandler);
        if (IfPush(i)){
                    // PRINT("KEY:%d\n", i);
                    
                }
        KeyFilter(IfPush(i), &keyStateList[i], i, KeyEventCallback);
    }
    // PRINT("key scan\n");
    MultiTimerStart(&keyTimer, SCAN_PERIOD, KeyScan, NULL);
}

void KeyEventCallback(int keyValue, enum KeyEvent event)
{
    keyEventHandler = TestOpenDoorHandler;
    keyEventHandler(keyValue,event);
    // PRINT("KeyValue = %d,event = %d\n",keyValue,event);
}

void TestOpenDoorHandler(int keyValue, uint8_t event){
    uint8_t temp[] = {REGISTER_CMD,RESTORE_FACTORY_CMD,MODE_SWITCH};
    if (event == RELEASE_EVENT) {
        // 短按触发以后，callback不为NULL，长按触发以后callback=NULL，所以这里时短按释放
        if (keyValue == KEY_SET && setKeyLongPushTimer.callback) {
            MultiTimerStop(&setKeyLongPushTimer);
            UART2_SendData(temp);
        } else if (keyValue == KEY_MODE && modeKeyLongPushTimer.callback) {
            MultiTimerStop(&modeKeyLongPushTimer);
        }
    } else if (event == PUSH_EVENT) {
        if (keyValue == KEY_SET) {
            MultiTimerStart(&setKeyLongPushTimer, 3000, LongPushDetect, NULL);
        } else if (keyValue == KEY_MODE) {
            MultiTimerStart(&modeKeyLongPushTimer, 3000, LongPushDetect, NULL);
        }
    } else if (event == LONG_TRIGGER) {
        if (keyValue == KEY_SET) {
            setKeyLongPushTimer.callback = NULL;
            UART2_SendData(&temp[1]);
        } else if (keyValue == KEY_MODE) {
            UART2_SendData(&temp[2]);
            modeKeyLongPushTimer.callback = NULL;
        }
    }
}

void LongPushDetect(MultiTimer* timer, void* userData)
{
    if (timer == &setKeyLongPushTimer) {
        keyEventHandler(KEY_SET, LONG_TRIGGER);
    } else if (timer == &modeKeyLongPushTimer) {
        keyEventHandler(KEY_MODE, LONG_TRIGGER);
    }
}

