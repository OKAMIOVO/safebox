#ifndef __LOG_H__
#define __LOG_H__
#include <stdio.h>
#include <stdint.h>

#ifdef FUNC_LOG
void UART2_SendBuf(uint8_t* buf, int len);
void SendBuf(uint8_t* buf, int len);
#define PRINT(X...)                                    \
    {                                                  \
        char __debugStr[100];                          \
        int __debugStrLen = sprintf(__debugStr, X);    \
        SendBuf((uint8_t*)__debugStr, __debugStrLen); \
    }

#define PRINT2(X...)                                    \
    {                                                  \
        char __debugStr[100];                          \
        int __debugStrLen = sprintf(__debugStr, X);    \
        UART2_SendBuf((uint8_t*)__debugStr, __debugStrLen); \
    }
#else
#define PRINT(X...)
#define PRIN2(X...)
#endif
void PrintfBuf(const uint8_t* buf, int len);
void Print2fBuf(const uint8_t* buf, int len);
#endif
