#include "device.h"
#include <stddef.h>
#include <stdint.h>
#include "MultiTimer.h"
#include <stdio.h>
static struct Device* deviceList = NULL;
struct DeviceMgr deviceMgr;
void RegisterToDeviceList(struct Device* device)
{
    struct Device** temp = &deviceList;
    while (*temp) {
        temp = &((*temp)->next);
    }
    *temp = device;
}
void DeviceInit()
{
    struct Device* temp = deviceList;
    for (; temp; temp = temp->next) {
        if (temp->init) {
            temp->init();
        }
    }
}
void SleepTimerCallBack(MultiTimer* timer, void* userData)
{
    PRINT("SLEEP\n");
    struct Device* temp = deviceList;
    for (; temp; temp = temp->next) {
        if (temp->sleep) {
            temp->sleep();
        }
    }
    if (deviceMgr.sleepAndAwake != NULL) {
        deviceMgr.sleepAndAwake();
    }
}
