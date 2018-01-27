#ifndef COOPMULTITASKING_COOPMULTITASKING_H_
#define COOPMULTITASKING_COOPMULTITASKING_H_

#if ! defined(ARDUINO_ARCH_SAMC) && ! defined(ARDUINO_ARCH_SAMD) && ! defined(ARDUINO_ARCH_SAML)
#error The CoopMultitasking library currently only supports ARM Cortex-M0/M0+ processors (e.g. development boards based on Atmel/Microchip SAM C, D, and L MCUs).
#endif

#include <cstdint>

#include "CoopMultitasking/Types.h"

namespace CoopMultitasking {

    /**
     * Yields control of the current fiber and allows the next fiber to run.
     *
     * Calls to this function from an interrupt handler (the function you pass to attachInterrupt()) are IGNORED.
     *
     * CoopMultitasking::yield() is called automatically whenever you call the Arduino functions ::delay() and
     * ::yield().
     *
     * CoopMultitasking::yield() MUST be periodically called, typically via the Arduino functions ::delay() or
     * ::yield(); if it isn't, the other fibers will never have a chance to run.
     */
    extern void yield();


    /**
     * Starts a new fiber that runs the provided loop.
     *
     * Calls to this function from an interrupt handler (the function you pass to attachInterrupt()) are IGNORED.
     *
     * The function you provide will immediately run, and continue running, until delay() or yield() is called.
     * Typically you will start loops in setup(), which means they will run before loop(). Make sure to use a delay()
     * or a yield() in EACH of your new loops (AND the normal Arduino loop()!), otherwise loop() and the rest of your
     * new loops will never have a chance to run.
     *
     * @param func The function to call repeatedly in a loop; it is akin to the loop() function you write in your
     *             Arduino sketch; e.g. void loop2() { ... }
     * @param stackSize The stack size for the new fiber. The actual allocated size will be slightly larger than the
     *                  requested size by about 40 bytes; however, your code should never expect to use more than the
     *                  requested size.
     * @return One of the CoopMultitasking::Result values:
     *             CoopMultitasking::Success -- No error; the loop was started.
     *             CoopMultitasking::OutOfMemory -- There wasn't enough memory to allocate the requested stack size.
     *                 You can try again with a smaller value.
     *             CoopMultitasking::NotAllowed -- startLoop() was called from an interrupt handler; calls from
     *                 handlers are ignored.
     * [TODO provide default stack size?]
     */
    extern Result startLoop( LoopFunc func, uint32_t stackSize );
}


#ifndef COOPMULTITASKING_NO_YIELD_DEFINE

// hardware/arduino/avr/cores/arduino/hooks.c defines yield() as a weak symbol, which allows us to supply an
// implementation here
extern "C" void yield() {
    CoopMultitasking::yield();
}

#elif ! defined(COOPMULTITASKING_SUPPRESS_WARNINGS) // intentionally undocumented (see below); define if the warning is getting annoying :) -- be careful using this for obvious reasons...
// Let's provide a warning in keeping with Arduino's "Be kind to the end user" philosophy, because there won't be an
// obvious indication of failure if someone skims the docs and mistakenly defines COOPMULTITASKING_NO_YIELD_DEFINE.
#warning COOPMULTITASKING_NO_YIELD_DEFINE is defined, so ::yield() will NOT call CoopMultitasking::yield(). Your code MUST explicitly call CoopMultitasking::yield(). See the documentation for details.
#endif

#endif // header guard
