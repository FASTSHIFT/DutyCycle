/**
 * @file cxx_stubs.cpp
 * @brief C++ runtime stubs for bare-metal GCC builds
 *
 * Provides minimal implementations of C++ runtime functions
 * to avoid linking the full libstdc++ (saves significant flash space).
 * Note: operator new/delete are provided by umm_malloc_port/new.cpp
 */

#include <cstdlib>
#include <cstdint>

namespace __gnu_cxx {

/**
 * @brief Minimal terminate handler (replaces verbose version, saves ~18KB)
 */
[[noreturn]] void __verbose_terminate_handler()
{
    __asm volatile("bkpt #0");
    while (1) {
        __asm volatile("nop");
    }
}

} // namespace __gnu_cxx

extern "C" {

/**
 * @brief Called when a pure virtual function is invoked
 */
void __cxa_pure_virtual()
{
    __asm volatile("bkpt #0");
    while (1) {
    }
}

/**
 * @brief Stub for __cxa_atexit (static destructor registration)
 */
int __cxa_atexit(void (*)(void*), void*, void*)
{
    return 0;
}

/**
 * @brief Guard for static local variable initialization
 */
int __cxa_guard_acquire(uint64_t* guard)
{
    return !*(char*)guard;
}

void __cxa_guard_release(uint64_t* guard)
{
    *(char*)guard = 1;
}

void __cxa_guard_abort(uint64_t*)
{
}

} // extern "C"
