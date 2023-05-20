#include "CMS32L051.h" // Device header
#include "safe_box.h"
#include "battery_level.h"
#include "device.h"
#include "log.h"

extern enum BATTERY_LEVEL BatteryLevel;
uint8_t batteryDetectFlag = 0;

void batteryDetectInit(void);
//void batteryDetectSleep(void);
extern void UART2_SendData(uint8_t *sendData);
extern void delayMS(uint32_t n);
struct Device batteryDetect = {NULL, batteryDetectInit, NULL};

void batteryDetectInit(void)
{
    //PRINT("BatteryLevel = %d\n", BatteryLevel);
        if (BatteryLevel == LEVEL_0 || BatteryLevel == LEVEL_1)
        {
            // SafeBoxFsm(LOW_BATTERY_ALARM, NULL);
            uint8_t battery[] = {LOW_BATTERY_ALARM};
            delayMS(500);
            UART2_SendData(battery);
        }
}

//void batteryDetectSleep(void){
//    batteryDetectFlag = 0;
//}
