#ifndef _QUEUE_H_
#define _QUEUE_H_
#include <stdint.h>
#include <stdlib.h>
struct Queue {
    uint8_t* arr;
    int16_t front, size, rear;
};

#define GetFront(q) (q.arr[q.front])
#define GetRear(q) (q.arr[q.rear])
#define InitQueue(q, n, addr) \
    do {                      \
        q.arr = addr;         \
        q.front = q.rear = 0; \
        q.size = n;           \
    } while (0);

#define IsEmpty(q) (q.front == q.rear)
//判满
#define IsFull(q) (q.front == ((q.rear + 1) % q.size))

#define Dequeue(q)         \
    do {                   \
        q.front++;         \
        q.front %= q.size; \
    } while (0);
#define Enqueue(q)        \
    do {                  \
        q.rear++;         \
        q.rear %= q.size; \
    } while (0);
#define GetFrontAndDequque(q, c) \
    do {                         \
        c = GetFront(q);         \
        Dequeue(q);              \
    } while (0);
//入队
#define EnqueueElem(q, c)    \
    do {                     \
        q.arr[q.rear++] = c; \
        q.rear %= q.size;    \
    } while (0);
#define ClearQueue(q)         \
    do {                      \
        q.rear = q.front = 0; \
    } while (0);
// #define GetUsedCapacity(q) (((q).rear + (q).size - (q).front) % (q).size)
// #define GetUnUsedCapacity(q) ((q).size - 1 - GetUsedCapacity(q))
#endif
