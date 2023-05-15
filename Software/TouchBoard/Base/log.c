#include "log.h"

void PrintfBuf(const uint8_t* buf, int len)
{
    int i = 0;
    while (i++ < len) {
        PRINT("%02x ", *buf++);
    }
    PRINT("\n");
}

