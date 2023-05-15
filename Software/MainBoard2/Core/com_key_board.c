// C LIB
#include <string.h>
// CHIP LIB
#include "CMS32L051.h"
#include "userdefine.h"
#include "sci.h"
// SELF DEF LIB
#include "queue.h"
#include "MultiTimer.h"
#include "cal.h"
#include "com.h"
#include "intp.h"
// PRJ HEADERS
#include "key_scan.h"
// extern func and vaviable
void KeyEventHandler(int keyValue, enum KeyEvent event);

#define DATA_BUF_LEN_MAX 10
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
struct Device comKeyBoard = { NULL, ComInit, ComSleep };

static MultiTimer comTimer;
void ComTask(MultiTimer* timer, void* userData);

void Uart2SendEndCallback()
{
    com.sendBusyFlag = 0;
}
void Uart2RxByteCallback()
{
    if (!IsFull(com.rxQueue)) {
        EnqueueElem(com.rxQueue, rxByte);
    }
    // com.rxBuf[com.rxCnt++]=rxByte;
    UART2_Receive(&rxByte, 1);
}

void ComInit()
{
    SystemCoreClockUpdate();
    UART2_Init(SystemCoreClock, 9600);
    UART2_Receive(&rxByte, 1);
    InitQueue(com.txQueue, DATA_FRAME_CNT_MAX, txDataFrame);
    com.sendBusyFlag = 0;
    com.waitReplyFlag = 0;
    com.toReplyFlag = 0;
    InitQueue(com.rxQueue, BUF_LEN_MAX, rxFifiData);
    com.rxCnt = 0;
    com.timeoutCnt = 0;
    MultiTimerStart(&comTimer, 10, ComTask, NULL);
}

void SendDataFrame(struct DataFrame* dataFrame)
{
    com.sendBuf[0] = 0xaa;
    com.sendBuf[1] = dataFrame->len;
    com.sendBuf[2] = dataFrame->cmd;
    memcpy(&com.sendBuf[3], dataFrame->dataBuf, dataFrame->len);
    com.sendBuf[dataFrame->len + 3] = BitXorCal(com.sendBuf + 1, dataFrame->len + 2);
    com.sendBuf[dataFrame->len + 4] = 0xbb;
    UART2_Send(com.sendBuf, dataFrame->len + 5);
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
#ifdef PRJ_KEY_BOARD
    if (buf[2] < 0x80)
#else
    if (buf[2] & 0x80)
#endif
    { // need reply to main board
        if (buf[2] == 0x90) {
            KeyEventHandler(buf[3], (enum KeyEvent)buf[4]);
        }
        com.toReplyFlag = 1;
        com.replyDataFrame.cmd = buf[2];
        com.replyDataFrame.len = 0;
    } else { // get reply from main board
        if (com.waitReplyFlag) {
            if (buf[2] == GetFront(com.txQueue).cmd) {
                com.waitReplyFlag = 0;
                com.timeoutCnt = 0;
                Dequeue(com.txQueue);
                if (IsEmpty(com.txQueue)) {
                    MultiTimerStart(&deviceMgr.timer, SLEEP_TIME, SleepTimerCallBack, NULL);
                }
            }
        }
    }
    return len + 5;
}

void ComTask(MultiTimer* timer, void* userData)
{
    while (com.rxCnt < BUF_LEN_MAX && !IsEmpty(com.rxQueue)) {
        GetFrontAndDequque(com.rxQueue, com.rxBuf[com.rxCnt]);
        com.rxCnt++;
        MultiTimerStart(&deviceMgr.timer, SLEEP_TIME, SleepTimerCallBack, NULL);
    }
    if (!com.toReplyFlag) {
        GeneralParse(RxHandler, com.rxBuf, &com.rxCnt);
    }
    if (!com.sendBusyFlag) {
        if (com.toReplyFlag) {
            com.toReplyFlag = 0;
            com.sendBusyFlag = 1;
            SendDataFrame(&com.replyDataFrame);
        } else if (com.waitReplyFlag) {
            com.timeoutCnt++;
            if (com.timeoutCnt == 10 || com.timeoutCnt == 20 || com.timeoutCnt == 30) {
                com.sendBusyFlag = 1;
                SendDataFrame(&GetFront(com.txQueue));
                if (com.timeoutCnt >= 30) {
                    com.timeoutCnt = 0;
                    com.waitReplyFlag = 0;
                    Dequeue(com.txQueue);
                    if (IsEmpty(com.txQueue)) {
                        MultiTimerStart(&deviceMgr.timer, SLEEP_TIME, SleepTimerCallBack, NULL);
                    }
                }
            }
        } else if (!IsEmpty(com.txQueue)) {
            com.sendBusyFlag = 1;
            com.waitReplyFlag = 1;
            SendDataFrame(&GetFront(com.txQueue)); // 收到回复再出队
        }
    }
    MultiTimerStart(&comTimer, 10, ComTask, NULL);
}

void ComSleep()
{
    UART2_Stop();
    INTP_Init(1 << 0,INTP_RISING);
    INTP_Start(1 << 0);
}
