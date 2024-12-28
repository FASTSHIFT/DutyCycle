#include "ButtonEvent.h"
#include <string.h>

#ifndef UINT32_MAX
#define UINT32_MAX 4294967295u
#endif

#ifdef ARDUINO
#include "Arduino.h"
ButtonEvent::getTickCallback_t ButtonEvent::getTickCallback = millis;
#else
ButtonEvent::getTickCallback_t ButtonEvent::getTickCallback = 0;
#endif

/**
 * @brief  按键事件构造函数
 * @param  longPressTime: 按键长按触发超时设置
 * @param  longPressTimeRepeat: 长按重复触发时间
 * @param  doubleClickTime: 双击间隔时间
 * @retval 无
 */
ButtonEvent::ButtonEvent(
    uint16_t longPressTime,
    uint16_t longPressTimeRepeat,
    uint16_t doubleClickTime)
    : _nowState(STATE_NO_PRESS)
    , _longPressTimeCfg(longPressTime)
    , _longPressRepeatTimeCfg(longPressTimeRepeat)
    , _doubleClickTimeCfg(doubleClickTime)
    , _lastLongPressTime(0)
    , _lastPressTime(0)
    , _lastClickTime(0)
    , _clickCnt(0)
    , _isLongPressedEventSend(false)
    , _isPressed(false)
    , _isClicked(false)
    , _isLongPressed(false)
    , _eventCallback(0)
    , _userData(0)
{
}

/**
 * @brief  获取与上次时间的时间差(带uint32溢出识别)
 * @param  actTick: 当前的时间戳
 * @param  prevTick: 上次的时间戳
 * @retval 时间差
 */
uint32_t ButtonEvent::getTickElaps(uint32_t actTick, uint32_t prevTick)
{
    if (actTick >= prevTick) {
        prevTick = actTick - prevTick;
    } else {
        prevTick = UINT32_MAX - prevTick + 1;
        prevTick += actTick;
    }

    return prevTick;
}

/**
 * @brief  监控事件，建议扫描周期10ms
 * @param  nowState: 当前按键状态
 * @retval 无
 */
void ButtonEvent::monitor(bool isPress)
{
    if (!_eventCallback || !getTickCallback) {
        return;
    }

    uint32_t actTick = getTickCallback();

    if (isPress && _nowState == STATE_NO_PRESS) {
        _nowState = STATE_PRESS;

        _isPressed = true;
        _lastPressTime = actTick;

        _eventCallback(this, EVENT_PRESSED);
        _eventCallback(this, EVENT_CHANGED);
    }

    if (_nowState == STATE_NO_PRESS) {
        return;
    }

    if (isPress) {
        _eventCallback(this, EVENT_PRESSING);
    }

    if (isPress && getTickElaps(actTick, _lastPressTime) >= _longPressTimeCfg) {
        _nowState = STATE_LONG_PRESS;

        if (!_isLongPressedEventSend) {
            _eventCallback(this, EVENT_LONG_PRESSED);
            _lastLongPressTime = actTick;
            _isLongPressed = _isLongPressedEventSend = true;
        } else if (getTickElaps(actTick, _lastLongPressTime) >= _longPressRepeatTimeCfg) {
            _lastLongPressTime = actTick;
            _eventCallback(this, EVENT_LONG_PRESSED_REPEAT);
        }
    } else if (!isPress) {
        _nowState = STATE_NO_PRESS;

        if (getTickElaps(actTick, _lastClickTime) < _doubleClickTimeCfg) {
            _clickCnt++;
            _eventCallback(this, EVENT_DOUBLE_CLICKED);
        }

        if (_isLongPressedEventSend) {
            _eventCallback(this, EVENT_LONG_PRESSED_RELEASED);
        }

        _isLongPressedEventSend = false;
        _isClicked = true;
        _lastClickTime = actTick;

        if (getTickElaps(actTick, _lastPressTime) < _longPressTimeCfg) {
            _eventCallback(this, EVENT_SHORT_CLICKED);
        }

        _eventCallback(this, EVENT_CLICKED);
        _eventCallback(this, EVENT_RELEASED);
        _eventCallback(this, EVENT_CHANGED);
    }
}
