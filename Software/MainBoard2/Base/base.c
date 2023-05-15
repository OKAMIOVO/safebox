#include <stdint.h>
#include "base.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
//buf compare
int BufCmp(const uint8_t* buf1, const uint8_t* buf2, int n)
{
	if(n<=0){
		return -1;
	}
    int i = 0;
    while (i++ < n) {
        if (*buf1++ != *buf2++) {
            return i;
        }
    }
    return 0;
}

// 0~9,0x30~0x39
// A~F,0X41~0X46
// a~f,0x61~0x66
int GetHexFromChar(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return 0;
}

void InvertedOrderBuf(const uint8_t* src, uint8_t* des, int size)
{
    int i = 0;
    while (i < size) {
        *des++ = *(src + size - 1 - i);
        ++i;
    }
}
