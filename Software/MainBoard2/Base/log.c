#include "log.h"

void PrintfBuf(const uint8_t* buf, int len)
{
	#ifdef FUNC_LOG
    int i = 0;
    while (i++ < len) {
        PRINT("%d ", *buf++);
    }
    PRINT("\n");
	#endif
}

