#include "log.h"

void PrintfBuf(const uint8_t* buf, int len)
{
	#ifdef 	FUNC_LOG	// SYS_NUM == 1  // FUNC_LOG
    int i = 0;
    while (i++ < len) {
        PRINT("%02x ", *buf++);
    }
    PRINT("\n");
	#endif
}

