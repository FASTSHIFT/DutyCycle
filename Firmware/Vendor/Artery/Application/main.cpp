/*
 * MIT License
 * Copyright (c) 2017 - 2022 _VIFEXTech
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "rtc.h"
#include <Arduino.h>

#define CONFIG_MOT_OUT1_PIN PA2
#define CONFIG_MOT_OUT2_PIN PA3
#define CONFIG_PWR_PIN PA7
#define CONFIG_KEY_DET_PIN PB0
#define CONFIG_BAT_DET_PIN PB1
#define CONFIG_BUZZ_PIN PB2

static void update_rtc_config()
{
    Serial.printf("Build: %s %s", __DATE__, __TIME__);

    int day, year;
    char month[4];

    sscanf(__DATE__, "%3s %d %d", month, &day, &year); // "Sep 29 2023"

    int month_int = 0;
    static const char* month_names[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    for (int i = 0; i < sizeof(month_names) / sizeof(month_names[0]); i++) {
        if (strcmp(month, month_names[i]) == 0) {
            month_int = i + 1;
            break;
        }
    }

    int hour, minute, second;
    sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second); // "14:55:30"

    RTC_SetTime(year, month_int, day, hour, minute, second);
}

static void motorWrite(int value)
{
    Serial.println(value);
    analogWrite(CONFIG_MOT_OUT1_PIN, value > 0 ? value : 0);
    analogWrite(CONFIG_MOT_OUT2_PIN, value < 0 ? -value : 0);
}

static void sweep_test()
{
    for (int i = -1000; i < 1000; i++) {
        motorWrite(i);
        delay(10);
    }

    for (int i = 1000; i > -1000; i--) {
        motorWrite(i);
        delay(10);
    }
}

static void key_test()
{
    Serial.printf("Key state: %d\r\n", digitalRead(CONFIG_KEY_DET_PIN));
}

static uint32_t getTimestamp(int hour, int minute, int second)
{
    return hour * 3600 + minute * 60 + second;
}

static int32_t timestampMap(int32_t x, int32_t hour_start, int32_t hour_end, int32_t min_out, int32_t max_out)
{
    int32_t min_in = getTimestamp(hour_start, 0, 0);
    int32_t max_in = getTimestamp(hour_end, 0, 0);

    if (max_in >= min_in && x >= max_in)
        return max_out;
    if (max_in >= min_in && x <= min_in)
        return min_out;

    if (max_in <= min_in && x <= max_in)
        return max_out;
    if (max_in <= min_in && x >= min_in)
        return min_out;

    /**
     * The equation should be:
     *   ((x - min_in) * delta_out) / delta in) + min_out
     * To avoid rounding error reorder the operations:
     *   (x - min_in) * (delta_out / delta_min) + min_out
     */

    int32_t delta_in = max_in - min_in;
    int32_t delta_out = max_out - min_out;

    return ((x - min_in) * delta_out) / delta_in + min_out;
}

static void adjustMotor()
{
    if (Serial.available() <= 0) {
        return;
    }

    char c = Serial.read();

    static int motorValue = 0;

    switch (c) {
    case 'U':
        motorValue += 100;
        break;
    case 'D':
        motorValue -= 100;
        break;
    case 'u':
        motorValue += 10;
        break;
    case 'd':
        motorValue -= 10;
        break;
    default:
        motorValue = 0;
        break;
    }

    motorWrite(motorValue);
}

#define MOTOR_VALUE_5AM 690
#define MOTOR_VALUE_7AM 520
#define MOTOR_VALUE_9AM 300
#define MOTOR_VALUE_12AM 0
#define MOTOR_VALUE_9PM -310
#define MOTOR_VALUE_12PM -490
#define MOTOR_VALUE_1AM -540
#define MOTOR_VALUE_5PM_DOWN -710

static void onTimerIrq()
{
    RTC_Calendar_TypeDef calendar;
    RTC_GetCalendar(&calendar);

    uint32_t curTimestamp = getTimestamp(calendar.hour, calendar.min, calendar.sec);
    Serial.printf("Current times: %04d-%02d-%02d %02d:%02d:%02d, timestamp: %d\r\n",
        calendar.year, calendar.month, calendar.day, calendar.hour, calendar.min, calendar.sec, curTimestamp);
    
//    static int timeIndex = 0;
//    static const int timeTable[] = { 5, 7, 9, 12, 21, 0, 1, 4 };
//    if (timeIndex++ >= sizeof(timeTable) / sizeof(timeTable[0])) {
//        timeIndex = 0;
//    }
//    curTimestamp = getTimestamp(timeTable[timeIndex], 0, 0);

    static int motorValue = 0;

    if (curTimestamp >= getTimestamp(5, 0, 0) && curTimestamp < getTimestamp(7, 0, 0)) {
        motorValue = timestampMap(curTimestamp, 5, 7, MOTOR_VALUE_5AM, MOTOR_VALUE_7AM);
    } else if (curTimestamp >= getTimestamp(7, 0, 0) && curTimestamp < getTimestamp(9, 0, 0)) {
        motorValue = timestampMap(curTimestamp, 7, 9, MOTOR_VALUE_7AM, MOTOR_VALUE_9AM);
    } else if (curTimestamp >= getTimestamp(9, 0, 0) && curTimestamp < getTimestamp(12, 0, 0)) {
        motorValue = timestampMap(curTimestamp, 9, 12, MOTOR_VALUE_9AM, MOTOR_VALUE_12AM);
    } else if (curTimestamp >= getTimestamp(12, 0, 0) && curTimestamp < getTimestamp(21, 0, 0)) {
        motorValue = timestampMap(curTimestamp, 12, 21, MOTOR_VALUE_12AM, MOTOR_VALUE_9PM);
    } else if (curTimestamp >= getTimestamp(21, 0, 0) && curTimestamp < getTimestamp(24, 0, 0)) {
        motorValue = timestampMap(curTimestamp, 21, 24, MOTOR_VALUE_9PM, MOTOR_VALUE_12PM);
    } else if (curTimestamp >= getTimestamp(0, 0, 0) && curTimestamp < getTimestamp(1, 0, 0)) {
        motorValue = timestampMap(curTimestamp, 0, 1, MOTOR_VALUE_12PM, MOTOR_VALUE_1AM);
    } else if (curTimestamp >= getTimestamp(1, 0, 0) && curTimestamp < getTimestamp(5, 0, 0)) {
        motorValue = timestampMap(curTimestamp, 1, 5, MOTOR_VALUE_1AM, MOTOR_VALUE_5PM_DOWN);
    }

    motorWrite(motorValue);
}

static void setup()
{
    Serial.begin(115200);
    pinMode(CONFIG_PWR_PIN, OUTPUT);
    digitalWrite(CONFIG_PWR_PIN, HIGH);

    pinMode(CONFIG_MOT_OUT1_PIN, PWM);
    pinMode(CONFIG_MOT_OUT2_PIN, PWM);
    pinMode(CONFIG_KEY_DET_PIN, INPUT_PULLUP);

    pinMode(CONFIG_BUZZ_PIN, OUTPUT);
    tone(CONFIG_BUZZ_PIN, 500, 100);

    RTC_Init();
    update_rtc_config();

    Timer_SetInterrupt(TIM3, 2 * 1000 * 1000, onTimerIrq);
    Timer_SetEnable(TIM3, true);

    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    nvic_irq_disable(SysTick_IRQn);
}

static void loop()
{
    (void)sweep_test;
    (void)key_test;
    (void)adjustMotor;
    __WFI();
}

/**
  * @brief  Main Function
  * @param  None
  * @retval None
  */
int main(void)
{
    Core_Init();
    setup();
    for (;;)
        loop();
}
