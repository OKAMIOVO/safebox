#ifndef __BASE_H__
#define __BASE_H__
#include <stdio.h>
#include "queue.h"
#include <math.h>
// cmp
int BufCmp(const uint8_t* str1, const uint8_t* str2, int n);
// io
int GetHexFromChar(char c);
void InvertedOrderBuf(const uint8_t* src, uint8_t* des, int size);
void DelFirstElem(uint8_t* buf,int n);
#endif
