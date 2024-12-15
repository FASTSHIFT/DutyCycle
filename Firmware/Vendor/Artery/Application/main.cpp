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
#include <Arduino.h>

#define CONFIG_MOT_IN1_PIN PB1
#define CONFIG_MOT_IN2_PIN PB2

static void setup()
{
    pinMode(CONFIG_MOT_IN1_PIN, PWM);
    pinMode(CONFIG_MOT_IN2_PIN, PWM);
}

static void motorWrite(int value)
{
    analogWrite(CONFIG_MOT_IN1_PIN, value > 0 ? value : 0);
    analogWrite(CONFIG_MOT_IN2_PIN, value < 0 ? -value : 0);
}

static void loop()
{
    for(int i = -100; i < 100; i++)
    {
        motorWrite(i);
        delay(100);
    }
    
    for(int i = 100; i > -100; i--)
    {
        motorWrite(i);
        delay(10);
    }
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
    for(;;)loop();
}
