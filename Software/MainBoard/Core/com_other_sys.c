#include <string.h>
#include "CMS32L051.h" // Device header
#include "userdefine.h"
#include "sci.h"
#include "gpio.h"
#include "intp.h"

#include "Multitimer.h"
#include "cal.h"
#include "com.h"
#include "queue.h"
#include "device.h"
#include "log.h"
#include "base.h"
#include "safe_box.h"
#define DATA_BUF_LEN_MAX 13
#define DATA_FRAME_CNT_MAX 5
#define BUF_LEN_MAX 32
struct DataFrame {
    uint8_t cmd;
    uint8_t len;
    uint8_t dataBuf[DATA_BUF_LEN_MAX];
};
struct Com {
    struct DataFrameQueue {
        struct DataFrame* arr;
        int16_t front, size, rear;
    } txQueue;
    uint8_t sendBusyFlag : 1;
    uint8_t waitReplyFlag : 1;
    uint8_t toReplyFlag : 1;
    uint8_t sendBuf[BUF_LEN_MAX];
    struct Queue rxQueue;
    uint8_t rxBuf[BUF_LEN_MAX];
    int rxCnt;
    struct DataFrame replyDataFrame;
    // MultiTimer timeoutTimer;
    int8_t timeoutCnt;
    // void (*timeoutHandler)(MultiTimer* timer,void* userData);
};
static struct Com com;
static struct DataFrame txDataFrame[DATA_FRAME_CNT_MAX];
static uint8_t rxFifiData[BUF_LEN_MAX];

static uint8_t rxByte;

void ComInit(void);
void ComSleep(void);
struct Device comOtherSys = { NULL, ComInit, ComSleep };

static MultiTimer comTimer;
void ComTask(MultiTimer* timer, void* userData);

void Uart1SendEndCallback()
{
    com.sendBusyFlag = 0;
}
void Uart1RxByteCallback()
{
    if (!IsFull(com.rxQueue)) {
        EnqueueElem(com.rxQueue, rxByte);
    }
    // com.rxBuf[com.rxCnt++]=rxByte;
    UART1_Receive(&rxByte, 1);
}

void ComInit()
{
    SystemCoreClockUpdate();
    UART1_Init(SystemCoreClock, 9600);
    UART1_Receive(&rxByte, 1);
    InitQueue(com.txQueue, DATA_FRAME_CNT_MAX, txDataFrame);
    com.sendBusyFlag = 0;
    com.waitReplyFlag = 0;
    com.toReplyFlag = 0;
    InitQueue(com.rxQueue, BUF_LEN_MAX, rxFifiData);
    com.rxCnt = 0;
    com.timeoutCnt = 0;
    MultiTimerStart(&comTimer, 10, ComTask, NULL);
}

static void SendDataFrame(struct DataFrame* dataFrame)
{
    PRINT("send to other sys,cmd:%02x,len:%d,buf:", dataFrame->cmd, dataFrame->len);
    PrintfBuf(dataFrame->dataBuf, dataFrame->len);
    com.sendBuf[0] = 0xaa;
    com.sendBuf[1] = dataFrame->len;
    com.sendBuf[2] = dataFrame->cmd;
    memcpy(&com.sendBuf[3], dataFrame->dataBuf, dataFrame->len);
    com.sendBuf[dataFrame->len + 3] = BitXorCal(com.sendBuf + 1, dataFrame->len + 2);
    com.sendBuf[dataFrame->len + 4] = 0xbb;
    UART1_Send(com.sendBuf, dataFrame->len + 5);
}
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
#if SYS_NUM == 1
    if (buf[2] & 0x80)
#else
    if ((buf[2] & 0x80) == 0)
#endif
    { // need reply to main board
        com.toReplyFlag = 1;
        com.replyDataFrame.cmd = buf[2];
        com.replyDataFrame.len = 0;
    } else { // get reply from main board
        if (com.waitReplyFlag) {
            if (buf[2] == GetFront(com.txQueue).cmd) {
                if (buf[2] == 0x50 && BufCmp(GetFront(com.txQueue).dataBuf, buf + 3, GetFront(com.txQueue).len) != 0) { // need confirm password identical
                    com.timeoutCnt = 10;
                } else {
                    com.waitReplyFlag = 0;
                    com.timeoutCnt = 0;
                    Dequeue(com.txQueue);
                    // SafeBoxFsm(PASSWORD_BACKUP_SUCCESS,NULL);
                }
            }
        }
    }
    return len + 5;
}

static void ComTask(MultiTimer* timer, void* userData)
{
    while (com.rxCnt < BUF_LEN_MAX && !IsEmpty(com.rxQueue)) {
        GetFrontAndDequque(com.rxQueue, com.rxBuf[com.rxCnt]);
        com.rxCnt++;
    }
    if (!com.toReplyFlag) {
        GeneralParse(RxHandler, com.rxBuf, &com.rxCnt);
    }
    if (!com.sendBusyFlag) {
        if (com.toReplyFlag) {
            PRINT("REPLY\n");
            com.toReplyFlag = 0;
            com.sendBusyFlag = 1;
            SendDataFrame(&com.replyDataFrame);
        } else if (com.waitReplyFlag) {
            com.timeoutCnt++;
            PRINT("wait reply\n");
            if (com.timeoutCnt == 10 || com.timeoutCnt == 20) {
                com.sendBusyFlag = 1;
                PRINT("retry\n");
                SendDataFrame(&GetFront(com.txQueue));
            } else if (com.timeoutCnt == 30) {
                PRINT("time out 3 times\n");
                com.timeoutCnt = 0;
                com.waitReplyFlag = 0;
                Dequeue(com.txQueue);
            }
        } else if (!IsEmpty(com.txQueue)) {
            com.sendBusyFlag = 1;
            com.waitReplyFlag = 1;
            PRINT("send\n");
            SendDataFrame(&GetFront(com.txQueue)); // 收到回复再出队
        }
    }
    else{
        PRINT("tx busy\n");
    }
    MultiTimerStart(&comTimer, 10, ComTask, NULL);
}

void ComSleep()
{
    UART1_Stop();
}

void SendPasswordToOtherSys(const uint8_t* password, int len)
{
    struct DataFrame temp;
    if ((len >= 6 && len <= 12) || len == 0) {
        temp.cmd = 0x50;
        temp.len = len + 1;
        temp.dataBuf[0] = len;
        memcpy(temp.dataBuf + 1, password, len);
        if (len) {
            PRINT("NEW PASS WORD:");
            PrintfBuf(password, len);
        } else {
            PRINT("CLR PASS WORD\n");
        }
        PRINT("before enque:front=%d,rear=%d\n", com.txQueue.front, com.txQueue.rear);
        EnqueueElem(com.txQueue, temp);
        PRINT("after enque:front=%d,rear=%d\n", com.txQueue.front, com.txQueue.rear);
    } else {
        PRINT("illegal len\n");
    }
}
