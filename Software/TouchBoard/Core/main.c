#include <stdint.h>
#include "CMS32L051.h"
#include "userdefine.h"
#include "gpio.h"
#include "sci.h"
#include "intp.h"
#include "MultiTimer.h"
#include <stdio.h>
#include "device.h"
#include "key_filter.h"
#include "iica.h"

uint8_t sleepFlag = 0;
uint8_t awakeFlag = 0;
void SysTickInit(void);
void SysTickSleep(void);
uint64_t GetSysMsCnt(void);
struct Device sysTick = {NULL, SysTickInit, SysTickSleep};

extern void KeyBoardSleep();
extern void VoiceSleep(void);
extern void LedSleep();
extern void SleepFpmBoard(void);
extern void clearQueue(void);
extern void FPM_ComSleep(void);
extern void SleepFpmBoard(void);
extern void mainComSleep(void);

extern void awakeFpm(void);

extern void CSK14_Init();
extern void mainComInit();
extern void VoiceInit(void);
extern void LedInit();
extern void ComInit(void);
extern void FPMDealInit(void);

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
    //    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk; // disable systick
    PRINT("systick sleep\n");
}
void SleepAndAwake()
{
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    CGC->PMUKEY = 0x192A;
    CGC->PMUKEY = 0x3E4F;
    CGC->PMUCTL = 1;
    __STOP();
	NVIC_SystemReset();
    CGC->PMUKEY = 0x192A;
    CGC->PMUKEY = 0x3E4F;
    CGC->PMUCTL = 0;
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    /* SysTickInit();
    mainComInit();
    CSK14_Init();
    VoiceInit();
    LedInit();
    ComInit(); */
    // awakeFpm();

    PRINT("awake!!\n");
    PORT_SetBit(PORT12, PIN4);
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
void ReportKeyEvent(int keyValue, enum KeyEvent event);
extern struct Device keyBoard;
extern struct Device comMainBoard;
extern struct Device voicePlayer;
extern struct Device comFingerprint;
extern struct Device led;
extern struct Device touchboard;

int main()
{
    RegisterToDeviceList(&sysTick);
    RegisterToDeviceList(&comMainBoard);
    RegisterToDeviceList(&keyBoard);
    RegisterToDeviceList(&voicePlayer);
    RegisterToDeviceList(&led);
    RegisterToDeviceList(&touchboard);
    RegisterToDeviceList(&comFingerprint);
    deviceMgr.sleepAndAwake = SleepAndAwake;
    deviceMgr.sleepTime = 10000;
    // UART2_Init(SystemCoreClock, 115200);

    DeviceInit();
    PORT_SetBit(PORT12, PIN4);

    PRINT("while 1\n");
    // ReportKeyEvent(0, 0);
    PRINT("while 1\n");
    // PORT_Init(PORT13,PIN7,INPUT);
    //	PORT_Init(PORT2,PIN0,OUTPUT);
    //			PORT_SetBit(PORT2,PIN0);
    //								PORT_Init(PORT4,PIN0,OUTPUT);
    //			PORT_ClrBit(PORT4,PIN0);
    PRINT("device init finish\n");
    while (1)
    {
        MultiTimerYield();
        if (sleepFlag == 1)
        {
//            SleepFpmBoard();
//            FPM_ComSleep();
            SysTickSleep();
            KeyBoardSleep();
            VoiceSleep();
            LedSleep();
            mainComSleep();
            
            
            sleepFlag = 0;
            awakeFlag = 1;
        }
        if (awakeFlag == 1)
        {
            deviceMgr.sleepAndAwake();
            awakeFlag = 0;
        }
    }
}
void SendBuf(uint8_t *buf, int len)
{
    while (g_uart2_tx_count)
        ;
    delayMS(1);

    UART2_Send(buf, len);
    while (g_uart2_tx_count)
        ;
    delayMS(1);
}
