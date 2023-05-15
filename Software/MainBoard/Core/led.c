#include <string.h>
// CHIP LIB
#include "CMS32L051.h"
#include "userdefine.h"
#include "sci.h"
#include "gpio.h"
// SELF DEF LIB
#include "MultiTimer.h"
#include "device.h"
#include "log.h"

void LedInit()
{
    PORT_Init(PORT12,PIN4,OUTPUT);
    PORT_SetBit(PORT12,PIN4);
}
void LedSleep()
{
    PORT_ClrBit(PORT12,PIN4);
}
struct Device led={NULL,LedInit,LedSleep};
