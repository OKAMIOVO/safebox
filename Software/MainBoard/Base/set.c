
#if 0
int FindInSet(uint8_t elem, const uint8_t* set, int size)
{
    int i = 0;
    while (i < size) {
        if (elem == *set++) {
            return i;
        }
        i++;
    }
    return -1;
}

void SawFsm(struct SawMgr* sawMgr, int eventID)
{
    if (sawMgr->state == WAIT_ACK) {
        if (eventID == GET_ACK) {
            sawMgr->state = TX_IDLE;
            sawMgr->txCnt = 0;
        }
    }
}
#endif
