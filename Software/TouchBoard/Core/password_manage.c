#include <stdint.h>
#include <string.h>

#include "CMS32L051.h" // Device header
#include "userdefine.h"
#include "flash.h"

#include "cal.h"
#include "base.h"

#include "log.h"
#define PASSWORD_RESSTORE_ADDR 0x500000
uint8_t passwordBuf[12];
int8_t passwordLen = 0;
int8_t fpmUserCnt = 0;

void ReadPassword()
{
    PRINT("READ pass word\n");
    uint8_t buf[16];
    uint32_t* temp = (uint32_t*)&buf[0];
    uint32_t* w_ptr = (uint32_t*)0x500000;
    int i = 0;
    for (i = 0; i < 4; i++) {
        *temp++ = *w_ptr++;
    }
    PRINT("read flash:");
    PrintfBuf(buf,16);
    int len = buf[1];
    if (buf[0] == 0x5a && buf[len + 2] == uint8SumCal(buf + 2, len) && buf[len + 3] == 0xa5) {
        passwordLen = buf[1];
        memcpy(passwordBuf, buf + 2, len);
    } else {
        memset(passwordBuf, 0xff, 12);
        passwordLen = 0;
    }
}
void RestorePassword(const uint8_t* password, int n)
{
    uint8_t restoreBuf[16];
    restoreBuf[0] = 0x5a;
    restoreBuf[1] = (uint8_t)n;
    memcpy(restoreBuf + 2, password, n);
    restoreBuf[n + 2] = uint8SumCal(password, n);
    restoreBuf[n + 3] = 0xa5;
    flash_write(PASSWORD_RESSTORE_ADDR, 16, restoreBuf);
}
void ClrPassword()
{
    memset(passwordBuf, 0xff, 12);
    //passwordLen = 0;
    EraseSector(PASSWORD_RESSTORE_ADDR);
    ReadPassword();
}
int IsPasswordCorrect(const uint8_t* inputBuf, int n)
{
    // PrintfBuf(inputBuf, n);
    int startAddr = 0;
    for (startAddr = 0; startAddr <= n - passwordLen; ++startAddr) {
        if (BufCmp(inputBuf + startAddr, passwordBuf, passwordLen) == 0) {
            return 1;
        }
    }
    return 0;
}
