#if 0
void SawTask(struct SawMgr* sawMgr)
{
    if (sawMgr->state == TX_IDLE) {
        if (sawMgr->txCnt > 0) {
            sawMgr->sendBuf(sawMgr->txBuf, sawMgr->txCnt);
            sawMgr->state = WAIT_ACK;
            sawMgr->waitAckTime = sawMgr->waitAckTimeSetting;
        }
    } else if (sawMgr->state == WAIT_ACK) {
        sawMgr->waitAckTime--;
        if (sawMgr->waitAckTime == 0) {
            if (sawMgr->timeoutCnt < sawMgr->timeoutCntMax) {
                sawMgr->timeoutCnt++;
                sawMgr->sendBuf(sawMgr->txBuf, sawMgr->txCnt);
                sawMgr->waitAckTime = sawMgr->waitAckTimeSetting;
            } else {
                sawMgr->timeoutCnt = 0;
                sawMgr->state = TX_IDLE;
                sawMgr->txCnt = 0;
                sawMgr->timeoutHandler();
            }
        }
    }
}


#endif
#include "com.h"
void GeneralParse(int (*rxHandler)(const uint8_t*, int), uint8_t* buf, int* pLen)
{
    int i = 0, ret = 0, headAddr = 0;
    while (headAddr < *pLen) {
        ret = rxHandler(buf + headAddr, *pLen - headAddr);
        if (ret <= 0) { // finded head && hasn't recv enough byte
            break;
        } else {
            headAddr += ret;
        }
    }
    /* 将缓冲区中剩余的字节移动到缓冲区的开头。如果headAddr大于0，则说明已经找到了消息头。在这种情况下，将*pLen减去headAddr，以便更新缓冲区中剩余的字节数。
    然后，使用一个循环将缓冲区中剩余的字节移动到缓冲区的开头。这样做是为了确保下一次调用GeneralParse函数时，缓冲区中的数据从消息头开始。 */
    if (headAddr > 0) {
        *pLen -= headAddr;
        for (i = 0; i < *pLen; ++i) {
            buf[i] = buf[i + headAddr];
        }
    }
}
