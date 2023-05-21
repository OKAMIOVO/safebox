#ifndef _SAFE_BOX_H_
#define _SAFE_BOX_H_
#include <stdint.h>
#include "CMS32L051.h" // Device header
#include "userdefine.h"
#include "gpio.h"
#include "intp.h"
// extern void (*keyEventHandler)(int value, uint8_t eventType);

/* enum {
    VIBRATION_ENABLE,
    VIBRATION_DISABLE
}; */
enum
{
    INIT_STATE,
    IDENTIFY_STATE,
    DOOR_OPEN_STATE,
    REGISTER_STATE,
    ALARM_STATE,
    SLEEP_STATE
};

enum {
    PASSWORD_WAY,
    FINGERPRINT_WAY
};
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
    START_UNLOCK,
    BEEP,
    VIBRATION_ENABLE,
    VIBRATION_DISABLE,
    SLEEP,
    BACK_UP
    // DOOR_OPEN_TIMEOUT
};
#define OPEN_LED() PORT_SetBit(PORT12, PIN4)
#define CLOSE_LED() PORT_ClrBit(PORT12, PIN4)
void SafeBoxFsm(uint8_t event, uint8_t* userData);
void LockInit(void);
extern uint8_t vibrationTestEnable;
#define ENABLE_VIBRATION_TEST() INTP_Start(1 << 1)
#define DISABLE_VIBRATION_TEST() INTP_Stop(1 << 1)
void StartUnlock(void);
void Beep(int time);
void CtrlTouchBoardLed(uint8_t* ledBuf);
void SendFpmCmd(uint8_t state,uint8_t data);
void PlayVoice(const uint8_t* buf, int n);
void SleepTouchBoard(void);

enum {
    FPM_START_IDENTIFY,
    FPM_STOP_IDENTIFY,
    FPM_START_REGISTER,
    FPM_CONTINUE_REGISTER,
    FPM_STORE,
    FPM_EXIT_REGISTER,
    FPM_CLR_CMD,
    FPM_SET_COLOR
};
extern uint8_t enrollTimes;
#define OFF 0
#define OPEN 1
#define RED 1
#define BLUE 2
#define GREEN 4
#define KEY_VALUE_REPORT 0X90
#define FPM_CNT_REPORT 0XA0
#define FP_INDENTIFY_RESULT 0XA1
#define FP_REG_GET_AND_GEN 0XA2
#define FP_REG_REG_AND_STORE 0XA3
#define VOICE_EVENT_REPORT 0XB0
#define LED_TOUCH_BOARD_NUM 4
void SendPasswordToOtherSys(const uint8_t* password, int len);
extern void (*keyEventHandler)(int value, uint8_t eventType);
void IdentifyKeyHandler(int keyValue, uint8_t event);
void DoorOpenKeyHandler(int keyValue, uint8_t event);
#endif
