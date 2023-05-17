#include "CMS32L051.h"
#include "MultiTimer.h"
#include "device.h"
#include "log.h"

extern void SleepFpmBoard(void);

void SleepTouchBoard(void){
    PRINT("Enter SleepTouchBoard\r\n");
    //SleepFpmBoard();
    MultiTimerStart(&deviceMgr.timer, 0, SleepTimerCallBack, NULL);
}