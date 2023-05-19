#include <stdint.h>
#include <string.h>

#include "key_filter.h"
#include "device.h"
#include "safe_box.h"
#include "password_manage.h"
#include "voice.h"
#include "base.h"
// #include "led.h"
#define KEY_SET 12
#define KEY_MODE 13
#define KEY_POUND 11
#define KEY_ASTER 10

#define LED_SET_NUM 0
#define LED_UNLOCK_NUM 1
#define LED_BATTERY_NUM 2
#define LED_LOCK_NUM 3
#define LED_TOUCH_BOARD_NUM 4
// uint8_t ledState[5] = { 0, 0, 0, 0, 0 };
#define LED_CNT 4
#define VOICE_CONTINUE 0xf2
#define VOICE_STOP 0xfe
extern uint8_t ledState[LED_CNT];

void OpenLed(uint8_t ledNum)
{
    if (ledState[ledNum] != 1)
    {
        ledState[ledNum] = 1;
        PRINT("led[%d]=1\n", ledNum);
    }
}
void CloseLed(uint8_t ledNum)
{
    if (ledState[ledNum])
    {
        ledState[ledNum] = 0;
        PRINT("led[%d]=0\n", ledNum);
    }
}

/* enum
{
    INIT_STATE,
    IDENTIFY_STATE,
    DOOR_OPEN_STATE,
    REGISTER_STATE,
    ALARM_STATE,
    SLEEP_STATE
}; */
enum
{
    WAIT_REGISTER,
    PASSWORD_REGISTER,
    FINGERPRINT_REGISTER
};
enum
{
    VIBRATION_ALARM,
    ERROR_ALARM
};
enum
{
    PORTABLE_MODE,
    SAFE_MODE = ~PORTABLE_MODE
};

enum
{
    WAIT_BOTH_INDETIFY,
    WAIT_PASSWORD_INDENTIFY,
    WAIT_FINGERPRINT_IDENTIFY
};
uint8_t sysState = INIT_STATE;
uint8_t registerState = WAIT_REGISTER;
uint8_t registerPasswordInputTimes;
uint8_t unlockMode = PORTABLE_MODE;
uint8_t identifyState = WAIT_BOTH_INDETIFY;
uint8_t passwordInputBuf[2][16];
int8_t passwordInputCnt = 0;
MultiTimer doorNotCloseTimer;
#define PASSWORD_FAIL_TIMES_MAX 3
#define FINGERPRINT_FAIL_TIMES_MAX 5
uint8_t passwordIdentifyFailTimes = 0;
uint8_t fingerprintIdentifyFailTimes = 0;
uint8_t alarmType = VIBRATION_ALARM;
void DoorNotCloseCallback(MultiTimer *timer, void *userData);
void TestCallback(MultiTimer *timer, void *userData);
MultiTimer doorNotCloseTimer;
void DoorNotCloseCallback(MultiTimer *timer, void *userData);
MultiTimer alarmTimer;
void AlarmCallback(MultiTimer *timer, void *userData);
uint8_t vibrationTestEnable = 0;
MultiTimer setKeyLongPushTimer, modeKeyLongPushTimer;
#define LONG_TRIGGER 2
void (*keyEventHandler)(int value, uint8_t eventType) = NULL;
void IdentifyKeyHandler(int keyValue, uint8_t event);
void DoorOpenKeyHandler(int keyValue, uint8_t event);
void WaitRegKeyHandler(int keyValue, uint8_t event);
void RegPasswordKeyHandler(int keyValue, uint8_t event);
void RegFpmKeyHandler(int keyValue, uint8_t event);
void LongPushDetect(MultiTimer *timer, void *userData);
void KeyEventDisp(int keyValue, uint8_t event);
MultiTimer restoreFactoryTimer;
void RestoreFactoryTimeoutCallback(MultiTimer *timer, void *userData);
void IdentifyPass(void);
uint8_t delayEvent, delayData[10];
extern void SetLedState(uint8_t Num, uint8_t state);
extern void UART2_SendData(uint8_t *sendData);
// extern void CtrlTouchBoardLed(uint8_t* ledBuf);
extern void SendFpmCmd(uint8_t state, uint8_t data);
extern void SleepFpmBoard(void);

void DelayExcuteCb(MultiTimer *timer, void *userData)
{
    SafeBoxFsm(delayEvent, delayData);
}
static void SafeSleep(MultiTimer *timer, void *userData)
{
    PRINT("Enter safesleep!\r\n");
    // SetLedState(LED_TOUCH_BOARD_NUM,OFF);
    // sysState = SLEEP_STATE;
    SleepTouchBoard();
}
MultiTimer sleepTimer;
void StartIdentify()
{
    OpenLed(LED_LOCK_NUM);
    CloseLed(LED_UNLOCK_NUM);
    PRINT("StartIdentify LED_UNLOCK_NUM Close!\n");
    // OpenLed(LED_TOUCH_BOARD_NUM);
    SetLedState(LED_TOUCH_BOARD_NUM, OPEN);
    // CtrlTouchBoardLed(ledState);
    SendFpmCmd(FPM_STOP_IDENTIFY, 0);
    SendFpmCmd(FPM_SET_COLOR, BLUE);
    sysState = IDENTIFY_STATE;
    keyEventHandler = IdentifyKeyHandler;
    ReadPassword();
    passwordInputCnt = 0;
    PRINT("keyEventHandler = IdentifyKeyHandler;\n");
    PRINT("password len=%d\n", passwordLen);
    restoreFactoryTimer.callback = NULL;
    SendFpmCmd(FPM_START_IDENTIFY, 0);
    MultiTimerStart(&sleepTimer,10000,SafeSleep,NULL);
}
void GotoAlarmState(uint8_t type)
{
    sysState = ALARM_STATE;
    alarmType = type;
    PRINT("fp ERROR alarm!\n");
    uint8_t temp = alarmType ? VOICE_ALARM2 : VOICE_ALARM1;
    uint8_t play[] = {temp, VOICE_CONTINUE};
    PLAY_VOICE_SEGMENT(play, 2);
    deviceMgr.sleepTime = 10000;
    PRINT("sysState = %d\n", sysState);
    MultiTimerStart(&alarmTimer, 60000, AlarmCallback, NULL);
}
void StartRegPassword()
{
}
void StartRegFpm()
{
}
void GotoDoorOpenedState()
{
    sysState = DOOR_OPEN_STATE;
    keyEventHandler = NULL; // DoorOpenKeyHandler
    PRINT("Door opened,keyEventHandler = DoorOpenKeyHandler\n");
    MultiTimerStop(&sleepTimer);
    MultiTimerStart(&doorNotCloseTimer, 60000, DoorNotCloseCallback, NULL);
    OpenLed(LED_UNLOCK_NUM);
    CloseLed(LED_LOCK_NUM);
    PRINT("GotoDoorOpenedState LED_LOCK_NUM Close!\n");
    // CtrlTouchBoardLed(ledState);
}
void LockInit()
{
    sysState = INIT_STATE;
    keyEventHandler = NULL;
    PRINT("keyEventHandler=NULL;\n");
    PRINT("state = INIT_STATE;\n");
    ReadPassword();
    doorNotCloseTimer.callback = NULL;
}
uint8_t enrollTimes = 1;
MultiTimer registerTimer;
void RegFpTimeoutCallback(MultiTimer *timer, void *userData)
{
    SafeBoxFsm(EXIT_REGISTER, NULL);
}
MultiTimer indentifyFailTimer;
void RestartIdentify(MultiTimer *timer, void *usrData)
{
    PRINT("RestartIdentify!\n");
    StartIdentify();
}
#define START 0
#define RESTART 1
void StartRegister(uint8_t entry)
{
    SetLedState(LED_TOUCH_BOARD_NUM, OPEN);
    uint8_t temp[7], voiceLen = 0;
    if (entry)
    {
        temp[voiceLen++] = VOICE_REGISTER;
        temp[voiceLen++] = VOICE_SUCCESS;
    }
    if (passwordLen && fpmUserCnt >= 20)
    {
        temp[voiceLen++] = VOICE_USER_IS_FULL;
        PRINT("USER IS FULL!\n");
    }
    else if (passwordLen)
    {
        sysState = REGISTER_STATE;
        registerState = FINGERPRINT_REGISTER;
        MultiTimerStart(&registerTimer, 10000, RegFpTimeoutCallback, NULL);
        enrollTimes = 1;
        SendFpmCmd(FPM_START_REGISTER, 0);
        keyEventHandler = RegFpmKeyHandler;
        PRINT("keyEventHandler = RegFpmKeyHandler;\n");
        temp[voiceLen++] = VOICE_PASSWORD;
        temp[voiceLen++] = VOICE_USER_IS_FULL;
        temp[voiceLen++] = VOICE_PLEASE;
        temp[voiceLen++] = VOICE_INPUT;
        temp[voiceLen++] = VOICE_FINGERPRINT;
    }
    else if (fpmUserCnt >= 20)
    {
        temp[voiceLen++] = VOICE_FINGERPRINT;
        temp[voiceLen++] = VOICE_USER_IS_FULL;
        temp[voiceLen++] = VOICE_PLEASE;
        temp[voiceLen++] = VOICE_INPUT;
        temp[voiceLen++] = VOICE_PASSWORD;
        sysState = REGISTER_STATE;
        registerState = PASSWORD_REGISTER;
        keyEventHandler = RegPasswordKeyHandler;
        PRINT("keyEventHandler = RegPasswordKeyHandler;\n");
    }
    else
    {
        temp[voiceLen++] = VOICE_PLEASE;
        temp[voiceLen++] = VOICE_INPUT;
        temp[voiceLen++] = VOICE_PASSWORD;
        temp[voiceLen++] = VOICE_OR;
        temp[voiceLen++] = VOICE_FINGERPRINT;
        sysState = REGISTER_STATE;
        registerState = WAIT_REGISTER;
        keyEventHandler = WaitRegKeyHandler;
        SendFpmCmd(FPM_START_REGISTER, 0);
        PRINT("keyEventHandler = WaitRegKeyHandler;\n");
    }
    PLAY_VOICE_SEGMENT(temp, voiceLen);
}

void SafeBoxFsm(uint8_t event, uint8_t *userData)
{
    PRINT("sysState=%d,event=%d\n", sysState, event);
    if (event == LOW_BATTERY_ALARM)
    {
        uint8_t temp[] = {VOICE_VOLTAGE_LOW, VOICE_PLEASE, VOICE_REPLACE_BATTERY};
        PLAY_VOICE_SEGMENT(temp, sizeof(temp));
        PRINT("battery voltage low!!!\n");
        OpenLed(LED_BATTERY_NUM);
        PRINT("LED_BATTERY_NUM open!!!\n");
        // CtrlTouchBoardLed(ledState);
    }

    switch (sysState)
    {
    case INIT_STATE:
    {
        // StartIdentify();
        if (event == READ_USER_LIST_FINISH) {
            // 是否需要保存用户列表
            fpmUserCnt = *userData;
            StartIdentify();
            PRINT("INTI FINISH\n");
        } else if (event == READ_USER_LIST_TIMEOUT) {
            // 播报故障语音
            uint8_t temp = VOICE_FAIL_SOUND;
            PLAY_VOICE_SEGMENT(&temp, 1);
        }
    }
    break;
    case IDENTIFY_STATE:
    {
        if (event == IDENTIFY_SUCCESS)
        {

            if (unlockMode == PORTABLE_MODE)
            {
                IdentifyPass();
            }
            else
            {
                PRINT("identifyState == WAIT_BOTH_INDETIFY\n");
                if (identifyState == WAIT_BOTH_INDETIFY)
                {
                    if (userData[0] == PASSWORD_WAY)
                    {
                        identifyState = WAIT_FINGERPRINT_IDENTIFY;
                        // PlayPleaseInputFpm();
                        PlayInputFpm();
                    }
                    else
                    {
                        identifyState = WAIT_PASSWORD_INDENTIFY;
                        PRINT("WAIT_PASSWORD_INDENTIFY\n");
                        PlayPleaseInputPassword();
                    }
                }
                else if (identifyState == WAIT_PASSWORD_INDENTIFY)
                {
                    PRINT("identifyState == WAIT_PASSWORD_INDENTIFY\n");
                    if (userData[0] == PASSWORD_WAY)
                    {
                        PRINT("IdentifyPass\n");
                        IdentifyPass();
                    }
                    else
                    {
                        PRINT("PlayPleaseInputPassword\n");
                        PlayPleaseInputPassword();
                    }
                }
                else if (identifyState == WAIT_FINGERPRINT_IDENTIFY)
                {
                    if (userData[0] == PASSWORD_WAY)
                    {
                        // PlayPleaseInputFpm();
                        PlayInputFpm();
                    }
                    else
                    {
                        IdentifyPass();
                    }
                }
            }
        }
        else if (event == IDENTIFY_FAIL)
        {
            MultiTimerStop(&sleepTimer);
            if (passwordLen == 0 && fpmUserCnt == 0)
            {
                IdentifyPass();
            }
            else
            {
                SendFpmCmd(FPM_STOP_IDENTIFY, 0);
                PRINT("IDENTIFY_FAIL FPM_STOP_IDENTIFY!\n");
                SendFpmCmd(FPM_SET_COLOR, RED);
                uint8_t temp[] = {VOICE_IDENTIFY, VOICE_FAIL};
                PLAY_VOICE_SEGMENT(temp, sizeof(temp));
                if (userData[0] == FINGERPRINT_WAY)
                {
                    fingerprintIdentifyFailTimes++;
                    if (fingerprintIdentifyFailTimes >= FINGERPRINT_FAIL_TIMES_MAX)
                    {
                        fingerprintIdentifyFailTimes = 0;
                        GotoAlarmState(ERROR_ALARM);
                    }
                }
                else if (userData[0] == PASSWORD_WAY)
                {
                    passwordIdentifyFailTimes++;
                    if (passwordIdentifyFailTimes >= PASSWORD_FAIL_TIMES_MAX)
                    {
                        passwordIdentifyFailTimes = 0;
                        SetLedState(LED_TOUCH_BOARD_NUM, OFF);
                        GotoAlarmState(ERROR_ALARM);
                    }
                }
                MultiTimerStart(&indentifyFailTimer, 1000, RestartIdentify, NULL);
            }
        }
        else if (vibrationTestEnable && event == VIBRATION_EVENT)
        {
            GotoAlarmState(VIBRATION_ALARM);
            MultiTimerStop(&sleepTimer);
        }
        else if (event == DOOR_OPEN)
        {
            MultiTimerStop(&sleepTimer);
            GotoDoorOpenedState();
        }
        else if (event == MOTOR_TASK_FINISH)
        {
            StartIdentify();
        }
    }
    break;
    case DOOR_OPEN_STATE:
    {
        if (event == MODE_SWITCH)
        {
            unlockMode = ~unlockMode;
            PRINT("mode switch\n");
            const uint8_t voiceBufMode[][2] = {
                {VOICE_PORTABLE_MODE},
                {VOICE_SAFE, VOICE_MODE},
            };
            PLAY_VOICE_SEGMENT(voiceBufMode[unlockMode & 0x01], 1 + (unlockMode & 0x01));
        }
        else if (event == REGISTER_CMD)
        {
            CloseLed(LED_UNLOCK_NUM);
            OpenLed(LED_SET_NUM);
            PRINT("REGISTER_CMD LED_SET_NUM OPEN!\n");
            // CtrlTouchBoardLed(ledState);
            StartRegister(START);

            //  开启触摸板灯，指纹头灯
        }
        else if (event == RESTORE_FACTORY_CMD)
        {
            uint8_t temp[] = {VOICE_RESTORE_FACTORY};
            PLAY_VOICE_SEGMENT(temp, sizeof(temp));
            OpenLed(LED_TOUCH_BOARD_NUM);
            // CtrlTouchBoardLed(ledState);
            keyEventHandler = NULL;
            PRINT("restore factory\n");
            MultiTimerStart(&restoreFactoryTimer, 3000, RestoreFactoryTimeoutCallback, NULL);
            //            SendPasswordToOtherSys(NULL, 0);
        }
        else if (/* event == PASSWORD_BACKUP_SUCCESS && */ restoreFactoryTimer.callback == RestoreFactoryTimeoutCallback)
        {
            fpmUserCnt = 0;
            SendFpmCmd(FPM_CLR_CMD, 0);
            ClrPassword();
            GotoDoorOpenedState();
            restoreFactoryTimer.callback = NULL;
            MultiTimerStop(&restoreFactoryTimer);
            uint8_t temp[] = {VOICE_RESTORE_FACTORY, VOICE_SUCCESS};
            PLAY_VOICE_SEGMENT(temp, sizeof(temp));
        }
        else if (event == RESTORE_FACTORY_FAIL)
        {
            uint8_t temp[] = {VOICE_RESTORE_FACTORY, VOICE_FAIL};
            PLAY_VOICE_SEGMENT(temp, sizeof(temp));
            GotoDoorOpenedState();
        }
        else if (event == DOOR_CLOSE)
        {
            OpenLed(LED_LOCK_NUM);
            CloseLed(LED_UNLOCK_NUM);
            SetLedState(LED_TOUCH_BOARD_NUM, OFF);
            PRINT("DOOR_OPEN_STATE DOOR_CLOSE LED_UNLOCK_NUM Close!\n");
            CloseLed(LED_TOUCH_BOARD_NUM);
            PRINT("DOOR_OPEN_STATE DOOR_CLOSE LED_TOUCH_BOARD_NUM Close!\n");
            // CtrlTouchBoardLed(ledState);
            deviceMgr.sleepTime = 10000;
            StartIdentify();
            PRINT("DOOR CLOSE\n");
            doorNotCloseTimer.callback = NULL;
            MultiTimerStop(&doorNotCloseTimer);
            // 通知触摸板休眠
            // 关灯
        }
    }
    break;
    case REGISTER_STATE:
    {
        if (event == EXIT_REGISTER)
        {
            CloseLed(LED_SET_NUM);
            SetLedState(LED_TOUCH_BOARD_NUM, OFF);
            PRINT("REGISTER_STATE EXIT_REGISTER LED_SET_NUM Close!\n");
            // CtrlTouchBoardLed(ledState);
            SendFpmCmd(FPM_EXIT_REGISTER, 0);
            GotoDoorOpenedState();
            uint8_t temp[] = {VOICE_EXIT_MENU};
            PLAY_VOICE_SEGMENT(temp, sizeof(temp));
            PRINT("EXIT register\n");
            PRINT("sysState = %d\n", sysState);
        }
        else if (event == DOOR_CLOSE)
        {
            CloseLed(LED_SET_NUM);
            PRINT("REGISTER_STATE DOOR_CLOSE LED_SET_NUM Close!\n");
            OpenLed(LED_LOCK_NUM);
            CloseLed(LED_UNLOCK_NUM);
            SetLedState(LED_TOUCH_BOARD_NUM, OFF);
            PRINT("REGISTER_STATE DOOR_CLOSE LED_UNLOCK_NUM Close!\n");
            // CtrlTouchBoardLed(ledState);
            SendFpmCmd(FPM_EXIT_REGISTER, 0);
            uint8_t temp[] = {VOICE_EXIT_MENU};
            PLAY_VOICE_SEGMENT(temp, sizeof(temp));
            deviceMgr.sleepTime = 10000;
            StartIdentify();
            PRINT("DOOR CLOSE\n");
            doorNotCloseTimer.callback = NULL;
            MultiTimerStop(&doorNotCloseTimer);
            PRINT("DOOR_CLOSE FPM_STOP_IDENTIFY\n");
            SendFpmCmd(FPM_STOP_IDENTIFY, 0);
            SendFpmCmd(FPM_SET_COLOR, OFF);
            // 通知触摸板休眠
        }
        else
        {
            if (registerState == WAIT_REGISTER && event == REG_ONCE_SUCCESS)
            {
                MultiTimerStart(&registerTimer, 10000, RegFpTimeoutCallback, NULL);
                registerState = FINGERPRINT_REGISTER;
                enrollTimes = 1;
                keyEventHandler = RegFpmKeyHandler;
                SendFpmCmd(FPM_START_REGISTER, 0);
                PRINT("keyEventHandler = RegFpmKeyHandler;\n");
            }
            else if (registerState == WAIT_REGISTER && event == REG_ONCE_FAIL)
            {
                MultiTimerStart(&registerTimer, 10000, RegFpTimeoutCallback, NULL);
                SendFpmCmd(FPM_START_REGISTER, 0);
                uint8_t temp[] = {VOICE_REGISTER, VOICE_FAIL};
                PLAY_VOICE_SEGMENT(temp, sizeof(temp));
            }
            else if (registerState == FINGERPRINT_REGISTER)
            {
                if (event == REG_ONCE_SUCCESS)
                {
                    MultiTimerStart(&registerTimer, 10000, RegFpTimeoutCallback, NULL);
                    enrollTimes++;
                    if (enrollTimes <= 5)
                    {
                        uint8_t temp[] = {VOICE_PLEASE, VOICE_AGAIN, VOICE_INPUT};
                        PLAY_VOICE_SEGMENT(temp, sizeof(temp));
                        SendFpmCmd(FPM_CONTINUE_REGISTER, enrollTimes);
                    }
                    else
                    {
                        SendFpmCmd(FPM_STORE, fpmUserCnt);
                    }
                }
                else if (event == REG_ONCE_FAIL || event == REG_ONCE_FAIL)
                {
                    MultiTimerStart(&registerTimer, 10000, RegFpTimeoutCallback, NULL);
                    enrollTimes = 1;
                    uint8_t temp[] = {VOICE_REGISTER, VOICE_FAIL, VOICE_PLEASE, VOICE_RENEW, VOICE_INPUT};
                    PLAY_VOICE_SEGMENT(temp, sizeof(temp));
                    SendFpmCmd(FPM_START_REGISTER, enrollTimes);
                }
                else if (event == REG_ONE_FINGER_SUCCESS)
                {
                    fpmUserCnt++;
                    StartRegister(RESTART);
                }
            }
        }
    }
    break;
    case ALARM_STATE:
    {
        if (event == IDENTIFY_SUCCESS)
        {
            PRINT("Enter ALARM_STATE IDENTIFY_SUCCESS!\n");
            StartIdentify();
            STOP_PLAY_VOICE_MULTISEGMENTS();
            PRINT("remove alarm\n");
        }
        else if (event == ALARM_TIME_ENOUGH)
        {
            PRINT("Enter ALARM_STATE ALARM_TIME_ENOUGH!\n");
            STOP_PLAY_VOICE_MULTISEGMENTS();
            // SetLedState(LED_TOUCH_BOARD_NUM,OPEN);
            StartIdentify();
            PRINT("time enough\n");
        }
    }
    break;
    default:
        break;
    }
}

void LongPushDetect(MultiTimer *timer, void *userData)
{
    if (timer == &setKeyLongPushTimer)
    {
        keyEventHandler(KEY_SET, LONG_TRIGGER);
    }
    else if (timer == &modeKeyLongPushTimer)
    {
        keyEventHandler(KEY_MODE, LONG_TRIGGER);
    }
}
void RestoreFactoryTimeoutCallback(MultiTimer *timer, void *userData)
{
    SafeBoxFsm(RESTORE_FACTORY_FAIL, NULL);
}
char *keyStr[] = {
    "*",
    "#",
    "SET",
    "MODE"};
void KeyEventDisp(int keyValue, uint8_t event)
{
#ifdef FUNC_LOG
    if (event == 0)
    {
        PRINT("PRESS:");
    }
    else if (event == 1)
    {
        PRINT("release:");
    }
    else if (event == 2)
    {
        PRINT("LONG PRESS:")
    }
    if (keyValue < 10)
    {
        PRINT("%d\n", keyValue);
    }
    else
    {
        PRINT("%s\n", keyStr[keyValue - 10]);
    }
#endif
}

void IdentifyPass()
{
    MultiTimerStop(&sleepTimer);
    uint8_t temp[] = {VOICE_IDENTIFY, VOICE_PASSED};
    PRINT("IdentifyPass\n");
    PLAY_VOICE_SEGMENT(temp, sizeof(temp));

    //    StartUnlock();
    uint8_t unlock[] = {START_UNLOCK};
    UART2_SendData(unlock);
    // LED
    uint8_t vibration[] = {VIBRATION_DISABLE};
    vibrationTestEnable = 0;
    // DISABLE_VIBRATION_TEST();
    PRINT("VIBRATION_DISABLE!\n");
    UART2_SendData(vibration);
    fingerprintIdentifyFailTimes = 0;
    passwordIdentifyFailTimes = 0;
    identifyState = WAIT_BOTH_INDETIFY;
    PRINT("IdentifyPass FPM_STOP_IDENTIFY\n");
    SendFpmCmd(FPM_STOP_IDENTIFY, 0);
    SendFpmCmd(FPM_SET_COLOR, GREEN);
}

void AlarmCallback(MultiTimer *timer, void *userData)
{
    PRINT("ALARM_TIME_ENOUGH!\n");
    PRINT("sysState = %d\n", sysState);
    SafeBoxFsm(ALARM_TIME_ENOUGH, NULL);
}
MultiTimer doorNotCloseTimer;
void DoorNotCloseCallback(MultiTimer *timer, void *userData)
{
    // Beep(200);
    uint8_t beep[] = {BEEP};
    UART2_SendData(beep);
    MultiTimerStart(&doorNotCloseTimer, 500, DoorNotCloseCallback, NULL);
}
void IdentifyKeyHandler(int keyValue, uint8_t event) // after register password finish
{
    KeyEventDisp(keyValue, event);
    if (event == PUSH_EVENT)
    {
        PlayWaterDrop();
        if (keyValue < 10)
        {
            if (passwordInputCnt < 16)
            {
                passwordInputBuf[0][passwordInputCnt++] = keyValue;
            }
            else
            {
                DelFirstElem(passwordInputBuf[0], 16);
                passwordInputBuf[0][15] = keyValue;
            }
        }
        else if (keyValue == KEY_POUND)
        {
            PRINT("identify...\n");
            uint8_t temp = PASSWORD_WAY;
            if (!passwordInputCnt)
            {
                PRINT("input len = 0!\n");
                PlayPleaseInputPassword();
            }
            else
            {
                PRINT("password input:");
                PrintfBuf(passwordInputBuf[0], passwordInputCnt);
                if (passwordInputCnt < 6)
                {
                    SafeBoxFsm(IDENTIFY_FAIL, &temp);
                    PRINT("INPUT TOO SHORT!\n");
                }
                else
                {
                    if (passwordLen)
                    {
                        PRINT("password restore:");
                        PrintfBuf(passwordBuf, passwordLen);
                        if (IsPasswordCorrect(passwordInputBuf[0], passwordInputCnt))
                        {
                            SetLedState(LED_TOUCH_BOARD_NUM, OFF);
                            PRINT("pass send IDENTIFY_SUCCESS\n");
                            SafeBoxFsm(IDENTIFY_SUCCESS, &temp);
                        }
                        else
                        {
                            PRINT("password not correct!\n");
                            SafeBoxFsm(IDENTIFY_FAIL, &temp);
                        }
                    }
                    else
                    { // 未注册密码时任何密码都验证失败
                        PRINT("no password user!\n");
                        SafeBoxFsm(IDENTIFY_FAIL, &temp);
                    }
                }
                passwordInputCnt = 0;
            }
        }
        else if (keyValue == KEY_ASTER)
        {
            if (passwordInputCnt == 0)
            {
                uint8_t vibration[] = {VIBRATION_ENABLE};
                vibrationTestEnable = 1;
                UART2_SendData(vibration);
                // ENABLE_VIBRATION_TEST();
                PRINT("open vibration test\n");
                PlayAlarmOpened();
            }
            else
            {
                passwordInputCnt = 0;
            }
        }
    }
    if (doorNotCloseTimer.callback)
    {
        MultiTimerStart(&doorNotCloseTimer, 60000, DoorNotCloseCallback, NULL);
    }
    if (sysState == IDENTIFY_STATE)
    {
        MultiTimerStart(&sleepTimer,10000,SafeSleep,NULL);
    }
}
void DoorOpenKeyHandler(int keyValue, uint8_t event)
{
    KeyEventDisp(keyValue, event);
    if (event == RELEASE_EVENT)
    {
        // 短按触发以后，callback不为NULL，长按触发以后callback=NULL，所以这里时短按释放
        if (keyValue == KEY_SET && setKeyLongPushTimer.callback)
        {
            MultiTimerStop(&setKeyLongPushTimer);
            SafeBoxFsm(REGISTER_CMD, NULL);
        }
        else if (keyValue == KEY_MODE && modeKeyLongPushTimer.callback)
        {
            MultiTimerStop(&modeKeyLongPushTimer);
        }
    }
    else if (event == PUSH_EVENT)
    {
        if (keyValue == KEY_SET)
        {
            MultiTimerStart(&setKeyLongPushTimer, 3000, LongPushDetect, NULL);
        }
        else if (keyValue == KEY_MODE)
        {
            MultiTimerStart(&modeKeyLongPushTimer, 3000, LongPushDetect, NULL);
        }
    }
    else if (event == LONG_TRIGGER)
    {
        if (keyValue == KEY_SET)
        {
            setKeyLongPushTimer.callback = NULL;
            SafeBoxFsm(RESTORE_FACTORY_CMD, NULL);
        }
        else if (keyValue == KEY_MODE)
        {
            SafeBoxFsm(MODE_SWITCH, NULL);
            modeKeyLongPushTimer.callback = NULL;
        }
    }
}
void WaitRegKeyHandler(int keyValue, uint8_t event)
{
    PRINT("register\n");
    SetLedState(LED_TOUCH_BOARD_NUM, OPEN);
    MultiTimerStart(&registerTimer, 10000, RegFpTimeoutCallback, NULL);
    KeyEventDisp(keyValue, event);
    if (event == PUSH_EVENT)
    {
        PlayWaterDrop();
        if (keyValue < 10)
        {
            registerState = PASSWORD_REGISTER;
            keyEventHandler = RegPasswordKeyHandler;
            PRINT("keyEventHandler = RegPasswordKeyHandler;\n");
            SendFpmCmd(FPM_EXIT_REGISTER, 0);
            registerPasswordInputTimes = 0;
            PRINT("registerState = PASSWORD_REGISTER;\n");
            memset(passwordInputBuf[0], 0xff, 16);
            memset(passwordInputBuf[1], 0xff, 16);
            passwordInputBuf[0][0] = keyValue;
            passwordInputCnt = 1;
        }
        else if (keyValue == KEY_POUND)
        {
            PlayPleaseInputAain();
        }
        else if (keyValue == KEY_ASTER)
        {
            PlayPleaseInputAain();
            SafeBoxFsm(EXIT_REGISTER, NULL);
        }
    }
    if (doorNotCloseTimer.callback)
    {
        MultiTimerStart(&doorNotCloseTimer, 60000, DoorNotCloseCallback, NULL);
    }
}

void RegPasswordKeyHandler(int keyValue, uint8_t event)
{
    MultiTimerStart(&registerTimer, 10000, RegFpTimeoutCallback, NULL);
    KeyEventDisp(keyValue, event);
    if (event == PUSH_EVENT)
    {
        PlayWaterDrop();
        if (keyValue < 10)
        {
            if (passwordInputCnt < 16)
            {
                passwordInputBuf[registerPasswordInputTimes][passwordInputCnt++] = keyValue;
            }
            else
            {
                DelFirstElem(passwordInputBuf[registerPasswordInputTimes], 16);
                passwordInputBuf[registerPasswordInputTimes][15] = keyValue;
            }
        }
        else if (keyValue == KEY_POUND)
        {
            if (passwordInputCnt < 6)
            { // 密码长度不够
                PlayPleaseRenewInput();
                PRINT("password len is too short!\n");
            }
            else
            {
                registerPasswordInputTimes++;
                if (registerPasswordInputTimes >= 2)
                {
                    if (BufCmp(passwordInputBuf[0], passwordInputBuf[1], passwordInputCnt) == 0)
                    {
                        RestorePassword(passwordInputBuf[0], passwordInputCnt);
                        ReadPassword();
                        //                        SendPasswordToOtherSys(passwordBuf, passwordLen);
                        // 通知备用板密码
                        PRINT("RestorePassword,registerState = FINGERPRINT_REGISTER;\n");
                        PlayPleaseInputFpm();
                        SetLedState(LED_TOUCH_BOARD_NUM, OPEN);
                        registerState = FINGERPRINT_REGISTER;
                        MultiTimerStart(&registerTimer, 10000, RegFpTimeoutCallback, NULL);
                        enrollTimes = 1;
                        keyEventHandler = RegFpmKeyHandler;
                        SendFpmCmd(FPM_START_REGISTER, 0);
                        PRINT("keyEventHandler = RegFpmKeyHandler;\n");
                    }
                    else
                    {
                        PlayRegisterFail();
                        SetLedState(LED_TOUCH_BOARD_NUM, OFF);
                        PRINT("RegisterFail\n");
                    }
                }
                else
                {
                    PlayPleaseInputAain();
                    PRINT("input 1st time finish\n");
                }
            }
            passwordInputCnt = 0;
        }
        else if (keyValue == KEY_ASTER)
        {
            if (passwordInputCnt > 0)
            {
                passwordInputCnt = 0;
            }
            else
            {
                SafeBoxFsm(EXIT_REGISTER, NULL);
            }
        }
    }
    if (doorNotCloseTimer.callback)
    {
        MultiTimerStart(&doorNotCloseTimer, 60000, DoorNotCloseCallback, NULL);
    }
}
void RegFpmKeyHandler(int keyValue, uint8_t event)
{
    KeyEventDisp(keyValue, event);
    MultiTimerStart(&registerTimer, 10000, RegFpTimeoutCallback, NULL);
    if (event == PUSH_EVENT)
    {
        PlayWaterDrop();
        if (keyValue == KEY_ASTER)
        {
            SafeBoxFsm(EXIT_REGISTER, NULL);
        }
    }
    if (doorNotCloseTimer.callback)
    {
        MultiTimerStart(&doorNotCloseTimer, 60000, DoorNotCloseCallback, NULL);
    }
}
