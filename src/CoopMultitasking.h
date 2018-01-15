#ifndef _COOPMULTITASKING_COOPMULTITASKING_H_
#define _COOPMULTITASKING_COOPMULTITASKING_H_

#if ! defined(ARDUINO_ARCH_SAMC) && ! defined(ARDUINO_ARCH_SAMD) && ! defined(ARDUINO_ARCH_SAML)
#error The CoopMultitasking library currently only supports ARM Cortex-M0/M0+ processors (e.g. development boards based on Atmel/Microchip SAM C, D, and L MCUs).
#endif

namespace CoopMultitasking {

    using LoopFunc = void (*)();

    /**
     * Yields control of the current thread and allows the next thread to run.
     *
     * Yield is called automatically whenever you call the Arduino functions delay() and yield().
     *
     * Yield MUST be periodically called, typically via the Arduino functions delay() or yield(); if it isn't, the
     * other threads will never have a chance to run.
     */
    extern void yield();

    /**
     * Starts a new thread that runs the provided loop.
     *
     * The function you provide will immediately run, and continue running, until delay() or yield() is called.
     * Typically you will start loops in setup(), which means they will run before loop(). Make sure to use a delay()
     * or a yield() in EACH of your new loops, otherwise loop() (and the rest of your new loops) will never have a
     * chance to run.
     *
     * @param func The function to call repeatedly in a loop; it is akin to the loop() function you write in your
     *             Arduino sketch; e.g. void loop2() { ... }
     * @param stackSize The stack size for the new thread. The actual allocated size will be slightly larger than the
     *                  requested size by about 40 bytes; however, your code should never expect to use more than the
     *                  requested size.
     * [TODO notes on stack size, and possibly provide a default]
     */
    extern void startLoop( LoopFunc func, uint32_t stackSize );
}

#endif
