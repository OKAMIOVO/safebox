#ifndef __FPM_DEAL_LINK_QUEUE_H__
#define __FPM_DEAL_LINK_QUEUE_H__

struct fpmDataFrame
{
    uint8_t cmd;
    uint8_t len;
    uint8_t *dataBuf;
};

typedef struct fpmLinkNode
{
    struct fpmDataFrame fpmNode;
    struct fpmLinkNode* next;
}fpmLinkNode;

typedef struct{
    fpmLinkNode *front,*rear;
}fpmLinkQueue;

void InitQueue_FPM(fpmLinkQueue *fpmQ);
uint8_t IsEmpty_FPM(fpmLinkQueue *fpmQ);
void EnQueue_FPM(fpmLinkQueue *fpmQ,struct fpmDataFrame fpmNode);
void DeQueue_FPM(fpmLinkQueue *fpmQ,struct fpmDataFrame* fpmNode);

#endif
