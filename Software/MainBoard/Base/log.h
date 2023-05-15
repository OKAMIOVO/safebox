#ifndef __LOG_H__
#define __LOG_H__
#include <stdio.h>
#include <stdint.h>

#ifdef FUNC_LOG		//SYS_NUM == 1  // FUNC_LOG
void SendBuf(uint8_t* buf, int len);
#define PRINT(X...)                                    \
    {                                                  \
        char __debugStr[100];                          \
        int __debugStrLen = sprintf(__debugStr, X);    \
        SendBuf((uint8_t*)__debugStr, __debugStrLen); \
    }
#else
#define PRINT(X...)
#endif
void PrintfBuf(const uint8_t* buf, int len);
#endif
