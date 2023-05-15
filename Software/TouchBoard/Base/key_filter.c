#include "key_filter.h"
#include <string.h>
void KeyFilter(int IsTouch, enum KeyState* pState, int keyValue, void (*callback)(int, enum KeyEvent event))
{
    switch (*pState) {
    case KEY_CHECK: {
        if (IsTouch) {
            *pState = KEY_CONFIRM;
        }
    } break;
    case KEY_CONFIRM: {
        if (IsTouch) {
            *pState = KEY_RELEASE;
            if (callback != NULL) {
                callback(keyValue, PUSH_EVENT);
            }
        } else {
            *pState = KEY_CHECK;
        }
    } break;
    case KEY_RELEASE: {
        if (!IsTouch) {
            *pState = KEY_CHECK;
            if (callback != NULL) {
                callback(keyValue, RELEASE_EVENT);
            }
        }
    } break;
    default:
        break;
    }
}
