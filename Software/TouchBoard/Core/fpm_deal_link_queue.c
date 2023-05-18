#include "CMS32L051.h"
#include "log.h"
#include "fpm_deal_link_queue.h"
#include "stdlib.h"

void InitQueue_FPM(fpmLinkQueue *fpmQ){
    fpmQ->front = fpmQ->rear = (fpmLinkNode *)malloc(sizeof(fpmLinkNode));
    fpmQ->front->next = NULL;
}

uint8_t IsEmpty_FPM(fpmLinkQueue *fpmQ){
    if(fpmQ->front == fpmQ->rear){
        PRINT("QUEUE IS EMPTY!\n");
        return 1;
    }else{
        PRINT("QUEUE IS NOT EMPTY!\n");
        return 0;
    }
}

void EnQueue_FPM(fpmLinkQueue *fpmQ,struct fpmDataFrame fpmNode){
    fpmLinkNode *newNode = (fpmLinkNode *)malloc(sizeof(fpmLinkNode));
    newNode->fpmNode = fpmNode;
    newNode->next = NULL;
    fpmQ->rear->next = newNode;
    fpmQ->rear = newNode;
}

void DeQueue_FPM(fpmLinkQueue *fpmQ,struct fpmDataFrame* fpmNode){
    if(fpmQ->front == fpmQ->rear){
        PRINT("QUEUE IS EMPTY!\n");
        return;
    }
    fpmLinkNode *frontNode = fpmQ->front->next;
    *fpmNode = frontNode->fpmNode;
    fpmQ->front->next = frontNode->next;
    if(fpmQ->rear == frontNode){
        fpmQ->front = fpmQ->rear;
    }
    free(frontNode);
    frontNode = NULL;
}

