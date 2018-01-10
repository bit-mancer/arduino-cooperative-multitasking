#include <cstdint>
#include <malloc.h>

#include <Arduino.h>

#include "CoopMultitasking.h"


namespace CoopMultitasking {

    /**
     *
     * NOTE: assembly code expects the initial part of this structure to be well-defined.
     */
    struct GreenThread {

        // BEGIN assembly code expects this ordering
        uint32_t sp;
        GreenThread* next;
        // END assembly code expects this ordering

        uint32_t id;
    };



    static GreenThread* g_currentThread = nullptr;

    class Init final {
    public:
        Init() {
            g_currentThread = new GreenThread;

            static uint32_t id = 0;
            g_currentThread->id = id++;

            g_currentThread->next = g_currentThread;
        }
    };

    static Init g_init;



    void yield() {
    }

} // namespace


#ifndef COOPMULTITASKING_NO_YIELD_DEF
// hardware/arduino/avr/cores/arduino/hooks.c defines yield() as a weak symbol, which allows us to supply an
// implementation here
extern "C" void yield() {
    CoopMultitasking::yield();
}
#endif
