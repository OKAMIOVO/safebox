#include <stdint.h>
#include <string.h>
#include "CMS32L051.h"
#include "userdefine.h"
#include "sci.h"
#include "MultiTimer.h"
#include "log.h"
#include "device.h"
#include "safe_box.h"

uint8_t mainSleepFlag = 0;
uint8_t awakeFlag = 0;

extern void Key_ComSleep(void);
extern void ComSleep(void);
extern void VibrationTestSleep(void);
extern void LedSleep();
extern void CloseTestSleep(void);
extern void KeyBoardSleep(void);
extern void BatterySleep(void);
void SysTickSleep(void);

extern void SysTickInit(void);
extern void MotorInit(void);
extern void KeyComInit(void);
extern void ComInit(void);
extern void VibrationTestInit(void);
extern void LedInit();
extern void CloseTestInit(void);
extern void KeyIoInit(void);
extern void BuzzerInit(void);
extern void BatteryInit(void);

uint64_t GetSysMsCnt(void);
struct Device sysTick = { NULL, SysTickInit, SysTickSleep };

void SysTickInit()
{
    uint32_t msCnt;
    SystemCoreClockUpdate();
    msCnt = SystemCoreClock / 1000;
    SysTick_Config(msCnt);
    MultiTimerInstall(GetSysMsCnt);
}
void SysTickSleep()
{
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk; // disable systick
}
void SleepAndAwake()
{
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    CGC->PMUKEY = 0x192A;
    CGC->PMUKEY = 0x3E4F;
    CGC->PMUCTL = 1;
    __STOP();
    // NVIC_SystemReset();
    
    CGC->PMUKEY = 0x192A;
    CGC->PMUKEY = 0x3E4F;
    CGC->PMUCTL = 0;
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    SysTickInit();
    MotorInit();
    KeyComInit();
    ComInit();
    VibrationTestInit();
    LedInit();
    CloseTestInit();
    KeyIoInit();
    BuzzerInit();
    //BatteryInit();
    // DeviceInit();
    // LockInit();
    deviceMgr.sleepTime = 10000;
}
static uint64_t sysMsCnt = 0;
volatile uint32_t g_ticks;
void delayMS(uint32_t n)
{
    g_ticks = n;
    while (g_ticks)
        ;
}
uint64_t GetSysMsCnt()
{
    return sysMsCnt;
}
void SysTick_Handler(void)
{
    g_ticks--;
    sysMsCnt++;
}

extern struct Device comKeyBoard;
extern struct Device comOtherSys;
extern struct Device motor;
extern struct Device vibrationTest;
extern struct Device led;
extern struct Device closeTest;
extern struct Device keyBoard;
extern struct Device buzzer;
extern struct Device battery;
int main()
{
    RegisterToDeviceList(&sysTick);
    RegisterToDeviceList(&motor);
    RegisterToDeviceList(&comKeyBoard);
    RegisterToDeviceList(&comOtherSys);

    RegisterToDeviceList(&vibrationTest);
    RegisterToDeviceList(&led);
    RegisterToDeviceList(&closeTest);
    RegisterToDeviceList(&keyBoard);
    RegisterToDeviceList(&buzzer);
    RegisterToDeviceList(&battery);

    deviceMgr.sleepAndAwake = SleepAndAwake;
    deviceMgr.sleepTime = 10000;
    DeviceInit();
   // LockInit();
    PRINT("device init finish\n");
    while (1) {
        MultiTimerYield();
        if (mainSleepFlag == 1)
        {
            
            LedSleep();
            VibrationTestSleep();
            CloseTestSleep();
            KeyBoardSleep();
            // BatterySleep();
            Key_ComSleep();
            SysTickSleep();
            
            
            mainSleepFlag = 0;
            awakeFlag = 1;
        }
        if (awakeFlag == 1)
        {
            deviceMgr.sleepAndAwake();
            awakeFlag = 0;
        }
    }
}

void SendBuf(uint8_t* buf, int len)
{
    while (g_uart2_tx_count)
        ;
    delayMS(1);
    UART2_Send(buf, len);
    while (g_uart2_tx_count)
        ;
    delayMS(1);
}
