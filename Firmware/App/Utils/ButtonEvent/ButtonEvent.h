/**
  ******************************************************************************
  * @file    ButtonEvent.h
  * @author  FASTSHIFT
  * @version V1.6.0
  * @date    2018-12-23
  * @brief   非阻塞式按键事件驱动库，支持短摁、长摁、释放、状态改变事件的识别。
  * @Upgrade 2019.6.18  添加按键双击事件、长按单次触发事件。
  * @Upgrade 2019.8.26  添加GetClicked、GetPressed、GetLongPressed非事件模式支持。
  * @Upgrade 2019.12.4  使用了新的事件回调设计，所有事件统一用EventAttach。
                        添加Click、LongPressed、LongPressRepeat事件。
  * @Upgrade 2020.6.12  合并ButtonEvent_Type至ButtonEvent类
  * @Upgrade 2021.3.22  添加EVENT_SHORT_CLICKED和EVENT_PRESSING
                        整理命名，优化不必要的标志位
                        EventMonitor()形参使用bool类型，去除NoPressState统一状态
  * @Upgrade 2021.5.12  添加EventType.inc,更优雅的 枚举+字符串 自动生成方式
                        FuncCallBack_t -> FuncCallback_t
  * @Upgrade 2023.1.9   添加UserData和自定义时间戳获取函数
  * @Upgrade 2023.9.27  使用驼峰命名法，去除priv结构体，规范构造函数
  ******************************************************************************
  * @attention
  * 需要提供一个精确到毫秒级的系统时钟，用户需要设置TickGetterCallback
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BUTTON_EVENT_H
#define __BUTTON_EVENT_H

#include <stdint.h>

class ButtonEvent {
public:
    typedef enum {
#define EVENT_DEF(evt) evt
#include "EventType.inc"
#undef EVENT_DEF
        _EVENT_LAST
    } Event_t;

    typedef void (*eventCallback_t)(ButtonEvent* btn, Event_t event);
    typedef uint32_t (*getTickCallback_t)(void);

public:
    ButtonEvent(
        uint16_t longPressTime = 500,
        uint16_t longPressTimeRepeat = 200,
        uint16_t doubleClickTime = 200);

    void setEventCallback(eventCallback_t callback)
    {
        _eventCallback = callback;
    }

    static void setTickGetterCallback(getTickCallback_t callback)
    {
        getTickCallback = callback;
    }

    void monitor(bool isPress);

    const char* getEventString(uint16_t event)
    {
        const char* eventStr[_EVENT_LAST] = {
#define EVENT_DEF(evt) #evt
#include "EventType.inc"
#undef EVENT_DEF
        };

        return (event < _EVENT_LAST) ? eventStr[event] : "EVENT_NOT_FOUND";
    }

    inline uint16_t getClickCnt()
    {
        uint16_t cnt = _clickCnt + 1;
        _clickCnt = 0;
        return cnt;
    }

    inline bool getClicked()
    {
        bool n = _isClicked;
        _isClicked = false;
        return n;
    }

    inline bool getPressed()
    {
        bool n = _isPressed;
        _isPressed = false;
        return n;
    }

    inline bool getLongPressed()
    {
        bool n = _isLongPressed;
        _isLongPressed = false;
        return n;
    }

    operator uint8_t()
    {
        return _nowState;
    };

    void setUserData(void* userData)
    {
        _userData = userData;
    }

    void* getUserData()
    {
        return _userData;
    }

private:
    typedef enum {
        STATE_NO_PRESS,
        STATE_PRESS,
        STATE_LONG_PRESS
    } State_t;

private:
    State_t _nowState;
    uint16_t _longPressTimeCfg;
    uint16_t _longPressRepeatTimeCfg;
    uint16_t _doubleClickTimeCfg;
    uint32_t _lastLongPressTime;
    uint32_t _lastPressTime;
    uint32_t _lastClickTime;
    uint16_t _clickCnt;
    bool _isLongPressedEventSend;
    bool _isPressed;
    bool _isClicked;
    bool _isLongPressed;

    static getTickCallback_t getTickCallback;
    eventCallback_t _eventCallback;
    void* _userData;

private:
    uint32_t getTickElaps(uint32_t actTick, uint32_t prevTick);
};

#endif
