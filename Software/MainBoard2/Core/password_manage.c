#include "CMS32L051.h" // Device header
#include "userdefine.h"
#include <stdint.h>
#include "cal.h"
#include <string.h>
#include "flash.h"
#include "base.h"
#include "key_scan.h"
#include "motor.h"
#include "log.h"
uint8_t passwordBuf[12] = { 0, 0, 0, 0, 0, 0 };
int8_t passwordCnt = 6;
uint8_t passwordInputBuf[16];
int8_t passwordInputCnt = 0;
#define PASSWORD_RESSTORE_ADDR 0x500000
void ReadPassword()
{
    uint8_t buf[16];
    uint32_t* temp = (uint32_t*)&buf[0];
    uint32_t* w_ptr = (uint32_t*)0x500000;
    int i = 0;
    for (i = 0; i < 4; i++) {
        *temp++ = *w_ptr++;
    }
    int len = buf[1];
    if (buf[0] == 0x5a && buf[len + 2] == uint8SumCal(buf + 2, len) && buf[len + 3] == 0xa5) {
        passwordCnt = buf[1];
        memcpy(passwordBuf, buf + 2, len);
    } else {
        memset(passwordBuf, 0xff, 12);
        passwordCnt = 0;
    }

    // printf("read buf:");
    // PrintfBuf(buf, 16);
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
    ReadPassword();
}

void ClrPassword()
{
    memset(passwordBuf, 0xff, 12);
    EraseSector(PASSWORD_RESSTORE_ADDR);
    ReadPassword();
}
int IsPasswordCorrect(const uint8_t* inputBuf, int n)
{
    // PrintfBuf(inputBuf, n);
	if(n<6){
		return 0;
		}
    int startAddr = 0;
    for (startAddr = 0; startAddr <= n - passwordCnt; ++startAddr) {
        if (BufCmp(inputBuf + startAddr, passwordBuf, passwordCnt) == 0) {
            return 1;
        }
    }
    return 0;
}
void DelFirstElem(uint8_t* buf,int n)
{
    int i=0;
    while(i<n-1){
        buf[i]=buf[i+1];
        i++;
    }
}
void KeyEventHandler(int keyValue, enum KeyEvent event)
{
    //PRINT2("keyValue = %d,event = %d\n",keyValue,event);
    if (event == PUSH_EVENT) {
        // printf("push %d\n",keyMap[chn]);
        if (keyValue < 10) {
          if (passwordInputCnt < 16) {
                passwordInputBuf[passwordInputCnt++] = keyValue;
            } else {
                DelFirstElem(passwordInputBuf, 16);
                passwordInputBuf[15] = keyValue;
            }  
        } else {
            if (keyValue == KEY_POUND) {
                PRINT2("verify:");
                PRINT2("password restore:");
                ReadPassword();
                Print2fBuf(passwordBuf, passwordCnt);
                PRINT2("password input:");
                Print2fBuf(passwordInputBuf, passwordInputCnt);
                if (IsPasswordCorrect(passwordInputBuf, passwordInputCnt)) {
                    StartUnlock();
                }
            }
            passwordInputCnt = 0;
        }
    }
}
