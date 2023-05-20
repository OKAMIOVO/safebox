#ifndef _DEVICE_H_
#define _DEVICE_H_
#include <stdint.h>
#include "MultiTimer.h"
struct Device {
    struct Device* next;
    void (*init)(void);
    void (*sleep)(void);
};
void RegisterToDeviceList(struct Device* device);
struct DeviceMgr{
    MultiTimer timer;
		void (*sleepAndAwake)(void);
};
void SleepTimerCallBack(MultiTimer* timer,void* userData);
extern struct DeviceMgr deviceMgr;
void DeviceInit(void);
#define SLEEP_TIME 10000
#endif
