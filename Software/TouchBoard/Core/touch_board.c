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

void SleepTouchBoard(){
    PRINT("Enter SleepTouchBoard\r\n");
    MultiTimerStart(&deviceMgr.timer, 0, SleepTimerCallBack, NULL);
}
