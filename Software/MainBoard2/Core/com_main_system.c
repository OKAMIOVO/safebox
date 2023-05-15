#include "CMS32L051.h" // Device header
#include "userdefine.h"
#include "sci.h"
#include "flash.h"
#include "gpio.h"
#include "intp.h"
#include "Multitimer.h"
#include "cal.h"
#include "com.h"
#include "queue.h"
#include <string.h>
#include "log.h"
#include "com_main_system.h"
#include "device.h"

static struct Queue rxQueue;
#define UART1_RX_SIZE 32
static uint8_t rxQueueBuf[UART1_RX_SIZE];
static uint8_t rxByte;
static uint8_t rxBuf[UART1_RX_SIZE];
static int rxCnt = 0;
static MultiTimer comTimer;
void ComMainSysInit(void);
void ComMainSysSleep(void);
struct Device comMainSys = {NULL,ComMainSysInit,ComMainSysSleep};
void RestorePassword(const uint8_t* password, int n);
void ClrPassword(void);
uint8_t txBuf[UART1_RX_SIZE] = "test uart\n";
static int RxHandler(const uint8_t* buf, int n)
{
    if (buf[0] != 0xaa) {
        return 1;
    }
    if (n < 2) {
        return 0;
    }
    if (buf[1] > 20) {
        return 1;
    }
    int len = buf[1];
    if (n < len + 5) {
        return 0;
    }
    if (buf[len + 4] != 0xbb || buf[len + 3] != BitXorCal(buf + 1, len + 2)) {
        return 1;
    }
    if (buf[2] == 0x50) {
        if (buf[3] >= 6 && buf[3] <= 12) { // 密码合法长度6~12
            int i = 0;
            for (i = 0; i < buf[3]; ++i) {
                if (buf[4 + i] >= 10) { // 合法值0~9
                    break;
                }
            }
            PRINT("i=%d\n",i);
            if (i == buf[3]) { // 无非法值
                RestorePassword(buf + 4, buf[3]); // 密码合法，保存密码
                PRINT("restore password\n");
            }
        } else if (buf[3] == 0) {
            ClrPassword();
            PRINT("clr password\n");
        }
        memcpy(txBuf, buf, len + 5);
        UART1_Send(txBuf, len + 5);
    }
    return len + 5;
}
static void ParseRxData(MultiTimer* timer, void* userData)
{

    while (rxCnt < UART1_RX_SIZE && !IsEmpty(rxQueue)) {
        GetFrontAndDequque(rxQueue, rxBuf[rxCnt]);
        rxCnt++;
        MultiTimerStart(&deviceMgr.timer, SLEEP_TIME, SleepTimerCallBack, NULL);
    }
    GeneralParse(RxHandler, rxBuf, &rxCnt);
    MultiTimerStart(&comTimer, 5, ParseRxData, NULL);
}
void ComMainSysSleep()
{
    UART1_Stop();
    INTP_Init(1 << 1, INTP_RISING); // if get  intp key board is awake
    INTP_Start(1 << 1);
}
void ComMainSysInit()
{
    UART1_Init(SystemCoreClock, 9600); // start uart rx
    UART1_Receive(&rxByte, 1);
    InitQueue(rxQueue, UART1_RX_SIZE, rxQueueBuf);
    MultiTimerStart(&comTimer, 5, ParseRxData, NULL);
}
void Uart1RxByteCallback()
{
    if (!IsFull(rxQueue)) {
        EnqueueElem(rxQueue, rxByte);
    }
    UART1_Receive(&rxByte, 1);
}


