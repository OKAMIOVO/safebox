#include <stdint.h>
#include "CMS32L051.h"
#include "userdefine.h"
#include "gpio.h"
#include "sci.h"
#include "intp.h"
#include "MultiTimer.h"
#include <stdio.h>
#include "device.h"
#include "log.h"

extern struct Device keyBoard;
extern struct Device comMainBoard;
void SysTickInit(void);
void SysTickSleep(void);
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
    CGC->PMUKEY = 0x192A;
    CGC->PMUKEY = 0x3E4F;
    CGC->PMUCTL = 0;
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    DeviceInit();
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
int main()
{
    RegisterToDeviceList(&sysTick);
    RegisterToDeviceList(&keyBoard);
    RegisterToDeviceList(&comMainBoard);
    deviceMgr.sleepAndAwake = SleepAndAwake;
    DeviceInit();
		PRINT("INIT FINISH\n");
    while (1) {
        MultiTimerYield();
    }
}

void SendBuf(uint8_t* buf, int len)
{
    while (g_uart2_tx_count)
        ;
    UART2_Send(buf, len);
    while (g_uart2_tx_count)
        ;
}
