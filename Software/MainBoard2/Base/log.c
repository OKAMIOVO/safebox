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

void Print2fBuf(const uint8_t* buf, int len)
{
	#ifdef FUNC_LOG
    int i = 0;
    while (i++ < len) {
        PRINT2("%d ", *buf++);
    }
    PRINT2("\n");
	#endif
}

