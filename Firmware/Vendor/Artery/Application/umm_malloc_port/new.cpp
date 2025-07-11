/*
 * MIT License
 * Copyright (c) 2021 - 2025 _VIFEXTech
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

#include "External/umm_malloc/src/umm_malloc.h"

typedef void* (*alloc_func_t)(size_t);

static void* first_alloc(size_t size);

static alloc_func_t alloc_func = first_alloc;

static void* first_alloc(size_t size)
{
    umm_init();
    alloc_func = umm_malloc;
    return umm_malloc(size);
}

void* operator new(size_t size)
{
    return alloc_func(size);
}

void* operator new[](size_t size)
{
    return alloc_func(size);
}

void operator delete(void* ptr)
{
    umm_free(ptr);
}

void operator delete[](void* ptr)
{
    umm_free(ptr);
}
