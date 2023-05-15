#include "cal.h"

int uint8SumCal(const uint8_t* buf, int cnt)
{
    int sum = 0, i = 0;
    while (i++ < cnt) {
        sum += *buf++;
    }
    return sum;
}
uint8_t BitXorCal(const uint8_t* p, int N)
{
    int i = 0;
    uint8_t fcs = 0;
    for (i = 0; i < N; ++i) {
        fcs ^= *(p + i);
    }
    return fcs;
}
