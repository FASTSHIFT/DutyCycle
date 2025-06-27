/*
 * MIT License
 * Copyright (c) 2025 _VIFEXTech
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

#include "HAL.h"

uint8_t HAL_PinRemap(uint8_t pin)
{
    static int8_t is1V1board = -1;
    if (is1V1board < 0) {
        pinMode(CONFIG_BUZZ_V1_2_PIN, INPUT);

        /**
         * For version v1.1, the hardware PB1 is connected to the battery level detection,
         * while for higher versions, it is connected to the Buzzer.
         * Through this pin, the hardware version can be distinguished
         */
        is1V1board = digitalRead(CONFIG_BUZZ_V1_2_PIN);

        HAL_LOG_INFO("V1.1 Board detected: %d", is1V1board);
    }

    if (!is1V1board) {
        return pin;
    }

    switch (pin) {
    case CONFIG_BUZZ_V1_2_PIN:
        return CONFIG_BUZZ_V1_1_PIN;

    case CONFIG_BATT_DET_V1_2_PIN:
        return CONFIG_BATT_DET_V1_1_PIN;

    case CONFIG_PWR_EN_V1_2_PIN:
        return CONFIG_PWR_EN_V1_1_PIN;

    case CONFIG_BUTTON_SEL_V1_2_PIN:
        return CONFIG_BUTTON_SEL_V1_1_PIN;

    default:
        break;
    }

    return pin;
}
