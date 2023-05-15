#ifndef _KEY_FILTER_H_
#define _KEY_FILTER_H_
#include <stdint.h>

enum KeyEvent {
    PUSH_EVENT,
    RELEASE_EVENT
};
enum KeyState {
    KEY_CHECK,
    KEY_CONFIRM,
    KEY_RELEASE
};
void KeyFilter(int IsTouch, enum KeyState* pState, int i, void (*callback)(int, uint8_t event));
#endif
