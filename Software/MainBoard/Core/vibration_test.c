#include "CMS32L051.h"
#include "userdefine.h"
#include "gpio.h"
#include "intp.h"
#include "device.h"
#include "log.h"
#include "safe_box.h"
void VibrationTestInit(void);
void VibrationTestSleep(void);
struct Device vibrationTest = { NULL, VibrationTestInit, VibrationTestSleep };
static MultiTimer vibrationTestTimer;
void VibrationTestCallback(MultiTimer* timer, void* userData);
extern void UART2_SendData(uint8_t* sendData);
uint8_t vibrationTestEnable = 0;
void VibrationTestInit()
{
    INTP_Init(1 << 1, INTP_BOTH);
    MultiTimerStart(&vibrationTestTimer, 10, VibrationTestCallback, NULL);
}
void VibrationTestCallback(MultiTimer* timer, void* userData)
{
    
    if (vibrationTestEnable) {		// 
        INTP_Start(1 << 1);
		// PRINT("enter vibrationTestCallBack!\n");
        if (g_intp1Taken) {	// 
            g_intp1Taken = 0;
			// PRINT("enter vibrationTestCallBack!\n");
            uint8_t vibration[] = {VIBRATION_EVENT};
            UART2_SendData(vibration);
            // SafeBoxFsm(VIBRATION_EVENT, NULL);
        }
    }else{
        INTP_Stop(1 << 1);
    }
    MultiTimerStart(&vibrationTestTimer, 10, VibrationTestCallback, NULL);
}

void VibrationTestSleep()
{
    if (vibrationTestEnable) {		// 
        INTP_Start(1 << 1);
    } else {
        INTP_Stop(1 << 1);
    }
}