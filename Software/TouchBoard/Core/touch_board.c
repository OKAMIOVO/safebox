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
#include "queue.h"

#define LED_CTRL 0x20
#define FPM_CTRL 0x30
#define VOICE_CMD 0x40
#define SLEEP_CMD 0x50
#define DATA_BUF_LEN_MAX 10
#define DATA_FRAME_CNT_MAX 20
#define BUF_LEN_MAX 32

void SendFpmCmd(uint8_t state, uint8_t data);
void FPMDealInit(void);
void dealWithFpmData(MultiTimer* timer, void* userData);

struct DataFrame {
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
    } fpmDataQueue;
    struct DataFrame fpmDataQueueArray;
    uint8_t sendBuf[BUF_LEN_MAX];
    struct Queue fpmDataDealQueue;
    uint8_t fpmRXBuf[BUF_LEN_MAX];
    int fpmRXCnt;
    int fpmDataCnt;
    struct DataFrame replyDataFrame;
    int8_t timeoutCnt;
};
static struct DataFrame txDataFrame[DATA_FRAME_CNT_MAX];
static uint8_t rxFifiData[BUF_LEN_MAX];
struct Com com;

static MultiTimer comTimer;
struct Device touchBoard = {NULL, FPMDealInit, NULL};

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

uint8_t fpmCnt = 0;

void FPMDealInit(void)
{
    PRINT("FPMDealInit!\n");
    InitQueue(com.fpmDataQueue, DATA_FRAME_CNT_MAX, txDataFrame);
    MultiTimerStart(&comTimer, 10, dealWithFpmData, NULL);
}

void dealWithFpmData(MultiTimer* timer, void* userData){
    PRINT("Enter dealWithFpmData!\n");
    while (fpmCnt < DATA_FRAME_CNT_MAX && !IsEmpty(com.fpmDataQueue)) {
        GetFrontAndDequque(com.fpmDataQueue, com.fpmDataQueueArray);
        PRINT("com.fpmDataQueueArray.cmd = %x",com.fpmDataQueueArray.cmd);
        if(com.fpmDataQueueArray.cmd == FPM_CTRL){
            if(com.fpmDataQueueArray.dataBuf[0] == FPM_START_IDENTIFY){
                fpmTask.startIdentifyFlag = 1;
            }else if(com.fpmDataQueueArray.dataBuf[0] == FPM_STOP_IDENTIFY){
                fpmTask.stopIdentifyFlag = 1;
            }else if(com.fpmDataQueueArray.dataBuf[0] == FPM_START_REGISTER){
                fpmTask.startRegisterFlag = 1;
            }else if(com.fpmDataQueueArray.dataBuf[0] == FPM_CONTINUE_REGISTER){
                fpmTask.continueRegisterFlag = 1;
                fpmTask.continueEnrollTimes = com.fpmDataQueueArray.dataBuf[1];
            }else if(com.fpmDataQueueArray.dataBuf[0] == FPM_STORE){
                fpmTask.storeFlag = 1;
                fpmTask.storeUserId = com.fpmDataQueueArray.dataBuf[1];
            }else if(com.fpmDataQueueArray.dataBuf[0] == FPM_EXIT_REGISTER){
                fpmTask.stopIdentifyFlag = 1;
            }else if(com.fpmDataQueueArray.dataBuf[0] == FPM_CLR_CMD){
                fpmTask.clrAllFpFlag = 1;
            }else if(com.fpmDataQueueArray.dataBuf[0] == FPM_SET_COLOR){
                fpmTask.setColorFlag = 1;
                fpmTask.setColor = com.fpmDataQueueArray.dataBuf[1];
            }
        }else if(com.fpmDataQueueArray.cmd == SLEEP_CMD){
                fpmTask.sleepFpmFlag = 1;
        }else if(com.fpmDataQueueArray.cmd == FPM_CNT_REPORT){
            uint8_t userData = com.fpmDataQueueArray.dataBuf[0];
            SafeBoxFsm(READ_USER_LIST_FINISH, &userData);
        }else if(com.fpmDataQueueArray.cmd == FP_INDENTIFY_RESULT){
            uint8_t temp = FINGERPRINT_WAY;
            if(com.fpmDataQueueArray.dataBuf[0] == 0){
                SafeBoxFsm(IDENTIFY_SUCCESS, &temp);
            }else{
                SafeBoxFsm(IDENTIFY_FAIL, &temp);
            }
        }else if(com.fpmDataQueueArray.cmd == FP_REG_GET_AND_GEN){
            if(com.fpmDataQueueArray.dataBuf[0] == 0){
                SafeBoxFsm(REG_ONCE_SUCCESS, NULL);
            }else{
                SafeBoxFsm(REG_ONCE_FAIL, NULL);
            }
        }else if(com.fpmDataQueueArray.cmd == FP_REG_REG_AND_STORE){
            if(com.fpmDataQueueArray.dataBuf[0] == 0){
                SafeBoxFsm(REG_ONE_FINGER_SUCCESS, NULL);
            }else{
                SafeBoxFsm(REG_ONE_FINGER_FAIL, NULL);
            }
        }

        fpmCnt++;


    MultiTimerStart(&comTimer, 1, dealWithFpmData, NULL);
}

}

void SendFpmCmd(uint8_t state, uint8_t data)
{
    PRINT("ENTER SendFpmCmd!\n");
    struct DataFrame temp;
    temp.cmd = FPM_CTRL;
    temp.len = 2;
    temp.dataBuf[0] = state;
    temp.dataBuf[1] = data;
    EnqueueElem(com.fpmDataQueue,temp);
}

void FpmUserReport(uint8_t cnt)
{
    PRINT("ENTER FpmUserReport!\n");
    struct DataFrame temp;
    temp.cmd = FPM_CNT_REPORT;
    temp.len = 1;
    temp.dataBuf[0] = (uint8_t)cnt;
    if (!IsFull(com.fpmDataQueue))
    {
        EnqueueElem(com.fpmDataQueue, temp);
    }
    else
    {
        PRINT("TX QUEUE IS FULL\n");
    }
}
void FpIdentifyResult(uint8_t result)
{
    PRINT("ENTER FpIdentifyResult!\n");
    struct DataFrame temp;
    temp.cmd = FP_INDENTIFY_RESULT;
    temp.len = 1;
    temp.dataBuf[0] = result;
    if (!IsFull(com.fpmDataQueue))
    {
        EnqueueElem(com.fpmDataQueue, temp);
    }
    else
    {
        PRINT("TX QUEUE IS FULL\n");
    }
}
void FpRegGenCharResult(uint8_t result)
{
    PRINT("ENTER FpRegGenCharResult!\n");
    struct DataFrame temp;
    temp.cmd = FP_REG_GET_AND_GEN;
    temp.len = 1;
    temp.dataBuf[0] = result;
    if (!IsFull(com.fpmDataQueue))
    {
        EnqueueElem(com.fpmDataQueue, temp);
    }
    else
    {
        PRINT("TX QUEUE IS FULL\n");
    }
}
void FpRegStroeResult(uint8_t result)
{
    PRINT("ENTER FpRegStroeResult!\n");
    struct DataFrame temp;
    temp.cmd = FP_REG_REG_AND_STORE;
    temp.len = 1;
    temp.dataBuf[0] = result;
    if (!IsFull(com.fpmDataQueue))
    {
        EnqueueElem(com.fpmDataQueue, temp);
    }
    else
    {
        PRINT("TX QUEUE IS FULL\n");
    }
}

void SleepTouchBoard(){
    PRINT("Enter SleepTouchBoard\r\n");
    MultiTimerStart(&deviceMgr.timer, 0, SleepTimerCallBack, NULL);
}
