#include "CMS32L051.h"
#include "userdefine.h"
#include "adc.h"
#include "gpio.h"

#include "device.h"
#include "log.h"

#include "safe_box.h"
void BatteryInit(void);
void BatterySleep(void);
struct Device battery = { NULL, BatteryInit, BatterySleep };
extern void UART2_SendData(uint8_t* sendData);
extern void delayMS(uint32_t n);

MultiTimer batteryTimer;
void BatteryTask(MultiTimer* timer, void* userData);
uint8_t BatteryVoltage;
uint8_t ProtectVoltageTriggerTimes;
uint8_t LowBatteryProtectionEnabled;
uint8_t PostLowBattery;
#define a2d_databuffer_vbat 0
#define VBAT_FULL_A2D 558 // 30V, A2D = (VBAT/11/5)*1024
#define VBAT_EMPTY_A2D 372 // 20V, A2D = (VBAT/11/5)*1024
enum {
    LEVEL_0 = 0,
    LEVEL_1 = 1,
    LEVEL_2,
    LEVEL_3,
    LEVEL_4,
    LEVEL_5,
    LEVEL_6,
    LEVEL_7
} BatteryLevel;
uint32_t a2d_data[1] = { 1400 };
void HardwareBatVoltageA2dFilter(uint16_t NewA2d)
{

    if (NewA2d > a2d_data[a2d_databuffer_vbat]) {
        if (NewA2d > (20 + a2d_data[a2d_databuffer_vbat])) {
            a2d_data[a2d_databuffer_vbat] += 10;
        } else if (NewA2d > (5 + a2d_data[a2d_databuffer_vbat])) {
            a2d_data[a2d_databuffer_vbat] += 2;
        }
    } else if (a2d_data[a2d_databuffer_vbat] > NewA2d) {
        if (a2d_data[a2d_databuffer_vbat] > (NewA2d + 20)) {
            a2d_data[a2d_databuffer_vbat] -= 10;
        } else if (a2d_data[a2d_databuffer_vbat] > (NewA2d + 5)) {
            a2d_data[a2d_databuffer_vbat] -= 2;
        }
    }

    a2d_data[a2d_databuffer_vbat] = NewA2d;
}
void HardwareBatteryMgr_Task(void)
{
    // BatteryVoltage = (a2d_data[a2d_databuffer_vbat]*3)/34;	//in 0.1v, R1=15K, R2=10K
    BatteryVoltage = (a2d_data[a2d_databuffer_vbat] * 33*5) /4096; // in 0.1v, R1=15K, R2=10K
    //BatteryVoltage += 1; // Battery voltage will drop 0.3V as serial connected SS34
                         // printf("voltage = %d\n",BatteryVoltage);
                         PRINT("V=%d\n",BatteryVoltage);
    if (BatteryVoltage > 55) {
        BatteryLevel = LEVEL_4;
    } else if (BatteryVoltage > 51) {
        BatteryLevel = LEVEL_3;
    } else if (BatteryVoltage > 46) {
        BatteryLevel = LEVEL_2;
    } else if (BatteryVoltage >= 43) {
        BatteryLevel = LEVEL_1; // 4.2V
    } else {
        BatteryLevel = LEVEL_0;
    }

    if (BatteryLevel == LEVEL_0) {
        if (ProtectVoltageTriggerTimes < 3) {
            ProtectVoltageTriggerTimes++;
        } else {
            LowBatteryProtectionEnabled = 1; // 开启低电压保护&三次轮询机会
        }
    } else {
        if (ProtectVoltageTriggerTimes > 0) {
            ProtectVoltageTriggerTimes--;
        } else {
            LowBatteryProtectionEnabled = 0;
        }
    }
}
void BatteryInit()
{
    ADC_Init();
    PORT_Init(PORT12, PIN3, INPUT);
    PORT_SetBit(PORT12, PIN3);
    int8_t i = 0;
    for (i = 0; i < 10; i++) {
        uint32_t temp_a2d;
        uint16_t get_value[8];
        temp_a2d = ADC_Converse(ADC_CHANNEL_0, sizeof(get_value) / sizeof(get_value[0]), get_value); // ADC 平均值
        ADC_Stop();
        PRINT("AD=%d\n", temp_a2d);
        HardwareBatVoltageA2dFilter(temp_a2d); // 滤波更新a2d_data[a2d_databuffer_vbat]
        HardwareBatteryMgr_Task();
    }  
if(BatteryLevel==LEVEL_0||BatteryLevel==LEVEL_1){
	// SafeBoxFsm(LOW_BATTERY_ALARM, NULL);
    uint8_t battery[] = {LOW_BATTERY_ALARM};
    delayMS(500);
    UART2_SendData(battery);
}	
}
void BatterySleep()
{
}
