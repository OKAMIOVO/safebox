#ifndef _PASSWORD_MANAGE_H_
#define _PASSWORD_MANAGE_H_
#include <stdint.h>
extern uint8_t passwordBuf[];
extern int8_t passwordLen;
extern int8_t fpmUserCnt;
void ReadPassword(void);
void RestorePassword(const uint8_t* password, int n);
void ClrPassword(void);
int IsPasswordCorrect(const uint8_t* inputBuf, int n);
#endif
