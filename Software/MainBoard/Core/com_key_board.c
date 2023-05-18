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
#include "key_filter.h"
#include "device.h"
#include "log.h"
// PRJ HEADERS
// extern func and vaviable
#include "safe_box.h"
#include "password_manage.h"
#define LED_CTRL 0x20
#define FPM_CTRL 0x30
#define VOICE_CMD 0x40
#define SLEEP_CMD 0x50
#define DATA_BUF_LEN_MAX 10
#define DATA_FRAME_CNT_MAX 20
#define BUF_LEN_MAX 32
struct DataFrame
{
    uint8_t cmd;
    uint8_t len;
    uint8_t dataBuf[DATA_BUF_LEN_MAX];
};
struct Com
{
    struct DataFrameQueue
    {
        struct DataFrame *arr;
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
struct Device comKeyBoard = {NULL, ComInit, ComSleep};

static MultiTimer comTimer;
void ComTask(MultiTimer *timer, void *userData);

static void SendDataFrame(struct DataFrame *dataFrame);
static int RxHandler(const uint8_t *buf, int n);
void UART2_ReceiveData();
extern uint8_t vibrationTestEnable;
uint8_t uart2RxFlag = 0;
const uint8_t *uart2RxBuff;

void Uart2SendEndCallback()
{
    com.sendBusyFlag = 0;
}
void Uart2RxByteCallback()
{
    if (!IsFull(com.rxQueue))
    {
        EnqueueElem(com.rxQueue, rxByte);
    }
    // com.rxBuf[com.rxCnt++]=rxByte;
    UART2_Receive(&rxByte, 1);
}

void ComInit()
{
    SystemCoreClockUpdate();
    UART2_Init(SystemCoreClock, 115200);
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

void UART2_SendData(uint8_t *sendData)
{
    struct DataFrame dataFrame;

    dataFrame.len = 0;
    dataFrame.cmd = sendData[0];
    SendDataFrame(&dataFrame);
}

static void SendDataFrame(struct DataFrame *dataFrame)
{
    PRINT("MAIN send cmd:%02x,len:%d,buf:\r\n", dataFrame->cmd, dataFrame->len);
    PrintfBuf(dataFrame->dataBuf, dataFrame->len);
    com.sendBuf[0] = 0xaa;
    com.sendBuf[1] = dataFrame->len;
    com.sendBuf[2] = dataFrame->cmd;
    memcpy(&com.sendBuf[3], dataFrame->dataBuf, dataFrame->len);
    com.sendBuf[dataFrame->len + 3] = BitXorCal(com.sendBuf + 1, dataFrame->len + 2);
    com.sendBuf[dataFrame->len + 4] = 0xbb;
    UART2_Send(com.sendBuf, dataFrame->len + 5);
}

static int RxHandler(const uint8_t *buf, int n)
{
    int len;
    uart2RxBuff = buf;
    if (uart2RxBuff == NULL || n <= 0)
    {
        return 0;
    }
    if (uart2RxFlag == 0)
    {
        if (uart2RxBuff[0] != 0xaa)
        {
            return 1;
        }
        if (n < 2)
        {
            return 0;
        }
        if (uart2RxBuff[1] > 20)
        {
            return 1;
        }
        len = uart2RxBuff[1];
        if (n < len + 5)
        {
            return 0;
        }
        if (uart2RxBuff[len + 4] != 0xbb || uart2RxBuff[len + 3] != BitXorCal(uart2RxBuff + 1, len + 2))
        {
            return 1;
        }
        uart2RxFlag = 1;
        UART2_ReceiveData();
        // PRINT("MAIN RxHandler run!\r\n");
        
        /*#ifdef PRJ_KEY_BOARD*/
        //    if (buf[2] < 0x80)
        //#else
        //    if (buf[2] & 0x80)
        //#endif
            {
                // // need reply to
                // if (buf[2] == KEY_VALUE_REPORT) {
                //     if (keyEventHandler != NULL) {
                //         keyEventHandler(buf[3], buf[4]);
                //     }
                // } else if (buf[2] == FPM_CNT_REPORT) {
                //     uint8_t userData = buf[3];
                //     // SafeBoxFsm(READ_USER_LIST_FINISH, &userData);
                // } else if (buf[2] == FP_INDENTIFY_RESULT) {
                //     uint8_t temp = FINGERPRINT_WAY;
                //     if (buf[3] == 0) {
                //         // SafeBoxFsm(IDENTIFY_SUCCESS, &temp);
                //     } else {
                //         // SafeBoxFsm(IDENTIFY_FAIL, &temp);
                //     }
                // } else if (buf[2] == FP_REG_GET_AND_GEN) {
                //     if (buf[3] == 0) {
                //         // SafeBoxFsm(REG_ONCE_SUCCESS, NULL);
                //     } else {
                //         // SafeBoxFsm(REG_ONCE_FAIL, NULL);
                //     }
                // } else if (buf[2] == FP_REG_REG_AND_STORE) {
                //     if (buf[3] == 0) {
                //         // SafeBoxFsm(REG_ONE_FINGER_SUCCESS, NULL);
                //     } else {
                //         // SafeBoxFsm(REG_ONE_FINGER_FAIL, NULL);
                //     }
                // }
                //com.toReplyFlag = 1;
                // com.replyDataFrame.cmd = buf[2];
                //com.replyDataFrame.len = 0;
            } /*else { // get reply from
                if (com.waitReplyFlag) {
                    if (buf[2] == GetFront(com.txQueue).cmd) {
                        com.waitReplyFlag = 0;
                        com.timeoutCnt = 0;
                        Dequeue(com.txQueue);
                        if (buf[2] == SLEEP_CMD) {
                            // MultiTimerStart(&deviceMgr.timer, 0, SleepTimerCallBack, NULL);
                        }
                    }
                }
            }*/
    }
    return len + 5;
}

void UART2_ReceiveData()
{
    if (uart2RxFlag == 1)
    {
        PRINT("MAIN recv cmd:%02x,len:%d,buf:\r\n", uart2RxBuff[2], uart2RxBuff[1]);
        PrintfBuf(uart2RxBuff + 3, uart2RxBuff[1]);
        if (uart2RxBuff[2] == START_UNLOCK)
        {
            PRINT("UNLOCK!\n");
            StartUnlock();
        }
        else if (uart2RxBuff[2] == BEEP)
        {
            Beep(200);
        }
        else if (uart2RxBuff[2] == VIBRATION_DISABLE)
        {
            vibrationTestEnable = 0;
        }
        else if (uart2RxBuff[2] == VIBRATION_ENABLE)
        {
            vibrationTestEnable = 1;
        }

        uart2RxFlag = 0;
    }
}

static void ComTask(MultiTimer *timer, void *userData)
{

    while (com.rxCnt < BUF_LEN_MAX && !IsEmpty(com.rxQueue))
    {
        GetFrontAndDequque(com.rxQueue, com.rxBuf[com.rxCnt]);
        com.rxCnt++;
    }
    if (!com.toReplyFlag)
    {
        //com.rxCnt是全局变量，把他的地址作为参数，交给GeneralParse函数处理
        GeneralParse(RxHandler, com.rxBuf, &com.rxCnt);
        // UART2_ReceiveData();
    }
    if (!com.sendBusyFlag)
    {
        if (com.toReplyFlag)
        {
            com.toReplyFlag = 0;
            com.sendBusyFlag = 1;
            SendDataFrame(&com.replyDataFrame);
        }
        else if (com.waitReplyFlag)
        {
            com.timeoutCnt++;
            if (com.timeoutCnt == 15 || com.timeoutCnt == 30)
            {
                com.sendBusyFlag = 1;
                SendDataFrame(&GetFront(com.txQueue));
            }
            else if (com.timeoutCnt >= 45)
            {
                com.timeoutCnt = 0;
                com.waitReplyFlag = 0;
                PRINT("TIME OUT 3 times\n");
                Dequeue(com.txQueue);
            }
        }
        else if (!IsEmpty(com.txQueue))
        {
            com.sendBusyFlag = 1;
            com.waitReplyFlag = 1;
            SendDataFrame(&GetFront(com.txQueue)); // 收到回复再出队
        }
    }
    MultiTimerStart(&comTimer, 1, ComTask, NULL);
}

void ComSleep()
{
    UART2_Stop();
    INTP_Init(1 << 0, INTP_RISING);
    INTP_Start(1 << 0);
}

/*void CtrlTouchBoardLed(uint8_t* ledBuf)
{
    struct DataFrame temp;
    temp.cmd = LED_CTRL;
    temp.len = 5;
    temp.dataBuf[0] = ledBuf[0];
    temp.dataBuf[1] = ledBuf[1];
    temp.dataBuf[2] = ledBuf[2];
    temp.dataBuf[3] = ledBuf[3];
    temp.dataBuf[4] = ledBuf[4];
    EnqueueElem(com.txQueue, temp);
}

void SendFpmCmd(uint8_t state, uint8_t data)
{
    struct DataFrame temp;
    temp.cmd = FPM_CTRL;
    temp.len = 2;
    temp.dataBuf[0] = state;
    temp.dataBuf[1] = data;
    EnqueueElem(com.txQueue, temp);
}
void SleepTouchBoard()
{
    struct DataFrame temp;
    temp.cmd = SLEEP_CMD;
    temp.len = 0;
    EnqueueElem(com.txQueue, temp);
}
void PlayVoice(const uint8_t* buf, int n)
{
    struct DataFrame temp;
    temp.cmd = VOICE_CMD;
    temp.len = n;
    memcpy(temp.dataBuf, buf, n);
    EnqueueElem(com.txQueue, temp);
} */
