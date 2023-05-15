#include "CMS32L051.h" // Device header
#include "userdefine.h"
#include "tim4.h"
#include "Multitimer.h"
#include "device.h"
#include <stdio.h>

#include "log.h"
void BuzzerInit(void);
struct Device buzzer = { NULL, BuzzerInit, NULL };
MultiTimer buzzerTimer;
void BeepEnough(MultiTimer* timer, void* userData)
{
    TM41_Channel_Stop(TM4_CHANNEL_3);
}
void BuzzerInit()
{
    TM41_SquareOutput(TM4_CHANNEL_3, 1000);
    TM41_Channel_Stop(TM4_CHANNEL_3);
}
void Beep(int time)
{
    TM41_Channel_Start(TM4_CHANNEL_3);
    MultiTimerStart(&buzzerTimer, time, BeepEnough, NULL);
}
