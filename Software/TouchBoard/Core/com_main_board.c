#include "CMS32L051.h"
#include "userdefine.h"
#include "sci.h"
#include "queue.h"
#include "MultiTimer.h"
#include <string.h>
#include "cal.h"
#include "com.h"
#include "device.h"
#include "log.h"
#include "key_filter.h"
#include "intp.h"
void StartIdentifyFp(void);
void StopIdentify(void);
void StartRegisterFp(uint8_t);
void ContinueRegisterFp(uint8_t);
void StoreFp(uint8_t userID);
void StopRegisterFp(void);
void DelAllUserFormFpm(void);
#define DATA_BUF_LEN_MAX 10
#define DATA_FRAME_CNT_MAX 5
#define BUF_LEN_MAX 32
#define DEF_VoiceSegmentEndFlag 0xFA

#define LED_CTRL 0x20
#define FPM_CTRL 0x30
#define VOICE_CMD 0x40
#define SLEEP_CMD 0x50

void PLAY_VOICE_MULTISEGMENTS_FIXED(const uint8_t BUFF[], int);
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
    int8_t timeoutCnt;
};
static struct Com com;
static struct DataFrame txDataFrame[DATA_FRAME_CNT_MAX];
static uint8_t rxFifiData[BUF_LEN_MAX];

static uint8_t rxByte;

static void ComInit(void);
static void ComSleep(void);
struct Device comMainBoard = {NULL, ComInit, NULL};

static MultiTimer comTimer;
void ComTask(MultiTimer *timer, void *userData);
extern void KeyEventCallback(int keyValue, enum KeyEvent event);
extern void SafeBoxFsm(uint8_t event, uint8_t *userData);
void SendDataFrame(struct DataFrame *dataFrame);
uint8_t uart2RxFlag = 0;
const uint8_t* uart2RxBuff;
void UART2_ReceiveData();

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
    PRINT("uart2 init\n");
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
    struct DataFrame *dataFrame = NULL;

    // 检查输入参数是否有效
    if (sendData == NULL)
    {
        return;
    }

    // 动态分配内存
    dataFrame = (struct DataFrame *)malloc(sizeof(struct DataFrame));
    if (dataFrame == NULL)
    {
        // 处理内存分配失败
        return;
    }

    dataFrame->len = 0;
    dataFrame->cmd = sendData[0];

    // 发送数据帧
    SendDataFrame(dataFrame);
    free(dataFrame);
}

void SendDataFrame(struct DataFrame *dataFrame)
{
    PRINT("TOUCH send cmd:%02x,len:%d,buf\n:", dataFrame->cmd, dataFrame->len);
    PrintfBuf(dataFrame->dataBuf, dataFrame->len);
    com.sendBuf[0] = 0xaa;
    com.sendBuf[1] = dataFrame->len;
    com.sendBuf[2] = dataFrame->cmd;
    memcpy(&com.sendBuf[3], dataFrame->dataBuf, dataFrame->len);
    com.sendBuf[dataFrame->len + 3] = BitXorCal(com.sendBuf + 1, dataFrame->len + 2);
    com.sendBuf[dataFrame->len + 4] = 0xbb;
    UART2_Send(com.sendBuf, dataFrame->len + 5);
}
#define LED_CTRL 0x20
#define FPM_CTRL 0x30
#define VOICE_CMD 0x40
#define SLEEP_CMD 0x50
enum
{
    FPM_START_IDENTIFY,
    FPM_STOP_IDENTIFY,
    FPM_START_REGISTER,
    FPM_CONTINUE_REGISTER,
    FPM_STORE,
    FPM_EXIT_REGISTER,
    FPM_CLR_CMD,
    FPM_SET_COLOR
};
struct FpmComTask
{
    uint8_t startIdentifyFlag : 1;
    uint8_t stopIdentifyFlag : 1;
    uint8_t startRegisterFlag : 1;
    uint8_t continueRegisterFlag : 1;
    uint8_t storeFlag : 1;
    uint8_t sleepFpmFlag : 1;
    uint8_t clrAllFpFlag : 1;
    uint8_t setColorFlag : 1;
    uint8_t setColor;
    uint8_t storeUserId;
    uint8_t continueEnrollTimes;
};
extern struct FpmComTask fpmTask;

void SetLedState(uint8_t Num, uint8_t state);
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
        // PRINT("Touch Uart2RxHandler run!\r\n");
        // if (uart2RxBuff[2] < 0x80)
        // {   // need reply to main board
            //     if (buf[2] == LED_CTRL) {
            //         int i = 0;
            //         for (i = 0; i < 5; ++i) {
            //             SetLedState(i, buf[3 + i]);
            //         }
            //     } else if (buf[2] == VOICE_CMD) {
            //         PLAY_VOICE_MULTISEGMENTS_FIXED(buf + 3, buf[1]);
            //     } else if (buf[2] == FPM_CTRL) {
            //         if (buf[3] == FPM_START_IDENTIFY) {
            //             fpmTask.startIdentifyFlag = 1;
            //         } else if (buf[3] == FPM_STOP_IDENTIFY) {
            //             fpmTask.stopIdentifyFlag = 1;
            //         } else if (buf[3] == FPM_START_REGISTER) {
            //             fpmTask.startRegisterFlag = 1;
            //         } else if (buf[3] == FPM_CONTINUE_REGISTER) {
            //             fpmTask.continueRegisterFlag = 1;
            //             fpmTask.continueEnrollTimes = buf[4];
            //         } else if (buf[3] == FPM_STORE) {
            //             fpmTask.storeFlag = 1;
            //             fpmTask.storeUserId = buf[4];
            //         } else if (buf[3] == FPM_EXIT_REGISTER) {
            //             fpmTask.stopIdentifyFlag = 1;
            //         } else if (buf[3] == FPM_CLR_CMD) {
            //             fpmTask.clrAllFpFlag = 1;
            //         } else if (buf[3] == FPM_SET_COLOR) {
            //             fpmTask.setColorFlag = 1;
            //             fpmTask.setColor = buf[4];
            //         }
            //     }else if(buf[2]==SLEEP_CMD){
            //         fpmTask.sleepFpmFlag = 1;
            //     }
            //     com.toReplyFlag = 1;
            //     com.replyDataFrame.cmd = buf[2];
            //     com.replyDataFrame.len = 0;
        // }
        /* else
        { // get reply from main board
            if (com.waitReplyFlag)
            {
                if (uart2RxBuff[2] == GetFront(com.txQueue).cmd)
                {
                    com.waitReplyFlag = 0;
                    Dequeue(com.txQueue);
                    com.timeoutCnt = 0;
                    if (IsEmpty(com.txQueue))
                    {
                        //  MultiTimerStart(&deviceMgr.timer, SLEEP_TIME, SleepTimerCallBack, NULL);
                    }
                }
            }
        } */
        
    }

    return len + 5;
}

void UART2_ReceiveData()
{
    if (uart2RxFlag == 1)
    {
        PRINT("TOUCH recv cmd:%02x,len:%d,buf:\r\n", uart2RxBuff[2], uart2RxBuff[1]);
        PrintfBuf(uart2RxBuff + 3, uart2RxBuff[1]);
        SafeBoxFsm(uart2RxBuff[2], NULL);

        uart2RxFlag = 0;
    }
}

void ComTask(MultiTimer *timer, void *userData)
{
    // PRINT("com task\n");
    while (com.rxCnt < BUF_LEN_MAX && !IsEmpty(com.rxQueue))
    {
        GetFrontAndDequque(com.rxQueue, com.rxBuf[com.rxCnt]);
        //   PRINT("%02X ",com.rxBuf[com.rxCnt]);
        //  if(com.rxBuf[com.rxCnt] <= 13){
        /* if(com.rxBuf[com.rxCnt] == 30){
            KeyEventCallback(12,0);
        }else if(com.rxBuf[com.rxCnt] == 31){
            KeyEventCallback(12,1);
        }else if(com.rxBuf[com.rxCnt] == 32){
            KeyEventCallback(13,0);
        } */
        // SafeBoxFsm(com.rxBuf[com.rxCnt],NULL);
        //  KeyEventCallback(com.rxBuf[com.rxCnt],0);
        //}

        com.rxCnt++;
        //     MultiTimerStart(&deviceMgr.timer, SLEEP_TIME, SleepTimerCallBack, NULL);
    }
    // PRINT("parse:\n");
    if (!com.toReplyFlag)
    {
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
            if (com.timeoutCnt == 10 || com.timeoutCnt == 20)
            {
                com.sendBusyFlag = 1;
                SendDataFrame(&GetFront(com.txQueue));
            }
            else if (com.timeoutCnt >= 30)
            {
                com.timeoutCnt = 0;
                com.waitReplyFlag = 0;
                Dequeue(com.txQueue);
                if (IsEmpty(com.txQueue))
                {
                    //    MultiTimerStart(&deviceMgr.timer, SLEEP_TIME, SleepTimerCallBack, NULL);
                }
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
#define KEY_VALUE_REPORT 0X90
#define FPM_CNT_REPORT 0XA0
#define FP_INDENTIFY_RESULT 0XA1
#define FP_REG_GET_AND_GEN 0XA2
#define FP_REG_REG_AND_STORE 0XA3
#define VOICE_EVENT_REPORT 0XB0

#define FP_IDENTIFY_TIMEOUT 0xff
void ReportKeyEvent(int keyValue, enum KeyEvent event)
{
    if (event == 0)
    {
        struct DataFrame temp;
        temp.cmd = KEY_VALUE_REPORT;
        temp.len = 2;
        temp.dataBuf[0] = (uint8_t)keyValue;
        temp.dataBuf[1] = (uint8_t)event;
        if (!IsFull(com.txQueue))
        {
            EnqueueElem(com.txQueue, temp);
        }
    }
}
/* void FpmUserReport(uint8_t cnt)
{
    struct DataFrame temp;
    temp.cmd = FPM_CNT_REPORT;
    temp.len = 1;
    temp.dataBuf[0] = (uint8_t)cnt;
    if (!IsFull(com.txQueue))
    {
        EnqueueElem(com.txQueue, temp);
    }
    else
    {
        PRINT("TX QUEUE IS FULL\n");
    }
}
void FpIdentifyResult(uint8_t result)
{
    struct DataFrame temp;
    temp.cmd = FP_INDENTIFY_RESULT;
    temp.len = 1;
    temp.dataBuf[0] = result;
    if (!IsFull(com.txQueue))
    {
        EnqueueElem(com.txQueue, temp);
    }
    else
    {
        PRINT("TX QUEUE IS FULL\n");
    }
}
void FpRegGenCharResult(uint8_t result)
{
    struct DataFrame temp;
    temp.cmd = FP_REG_GET_AND_GEN;
    temp.len = 1;
    temp.dataBuf[0] = result;
    if (!IsFull(com.txQueue))
    {
        EnqueueElem(com.txQueue, temp);
    }
    else
    {
        PRINT("TX QUEUE IS FULL\n");
    }
}
void FpRegStroeResult(uint8_t result)
{
    struct DataFrame temp;
    temp.cmd = FP_REG_REG_AND_STORE;
    temp.len = 1;
    temp.dataBuf[0] = result;
    if (!IsFull(com.txQueue))
    {
        EnqueueElem(com.txQueue, temp);
    }
    else
    {
        PRINT("TX QUEUE IS FULL\n");
    }
} */
/* void ComSleep()
{
    PRINT("com_main sleep\n");
    UART2_Stop();
    INTP_Init(1<<2,INTP_RISING);
    INTP_Start(1<<2);
} */
