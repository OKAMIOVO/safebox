#ifndef _DEVICE_H_
#define _DEVICE_H_
#include <string.h>
#include <stdint.h>
#include "MultiTimer.h"
#include "log.h"
struct Device {
    struct Device* next;
    void (*init)(void);
    void (*sleep)(void);
};
void RegisterToDeviceList(struct Device* device);
struct DeviceMgr{
    MultiTimer timer;
	void (*sleepAndAwake)(void);
    int sleepTime;
};
void SleepTimerCallBack(MultiTimer* timer,void* userData);
extern struct DeviceMgr deviceMgr;
void DeviceInit(void);
#endif
