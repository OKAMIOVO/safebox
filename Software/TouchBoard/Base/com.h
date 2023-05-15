#ifndef _COM_H_
#define _COM_H_
#include<stdint.h>
//struct SawMgr {
//    enum {
//        TX_IDLE,
//        WAIT_ACK
//    } state;
//    int16_t waitAckTimeSetting; //设定超时时间，比方说某帧数据发完以后等待3个周期没有数据则重发，则这里设置为3
//    int16_t waitAckTime; //应用层不用设置，每次发完数据以后sawMgr会把该数据设置为waitAckTimeSetting
//    int16_t timeoutCnt;
//    int16_t timeoutCntMax; //最大超时次数
//    void (*timeoutHandler)(void); //超过设定的次数后，停等协议退出管理
//    uint8_t* txBuf;
//    int16_t txCnt; // txCnt=0时表示无数据需要发
//    void (*sendBuf)(uint8_t* buf, int n);
//};

//void SawTask(struct SawMgr* sawMgr);
//void SawFsm(struct SawMgr* sawMgr, int eventID);

void GeneralParse(int (*rxHandler)(const uint8_t*, int), uint8_t* buf, int* pLen);
#endif
