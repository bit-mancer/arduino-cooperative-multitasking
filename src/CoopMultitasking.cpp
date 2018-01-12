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




    extern "C" {

        /**
         * Switch the thread context from the provided current thread, to the next thread in the thread list.
         *
         * @param current A reference to the currently-executing thread. The reference will be updated to point to the
         *                next thread. Typically you would pass a reference to the global current thread instance.
         */
        static void __attribute__((naked)) __attribute__((noinline)) switchContext( GreenThread*& current ) {

            (void)current; // compiler hint for unused parameter warning

            asm (
                // AAPCS states that a subroutine must preserve the contents of the registers r4-r8, r10, r11 and sp
                // (and r9 in PCS variants that designate r9 as v6). Register 9 is the platform register: it's meaning
                // is defined by the virtual platform. I haven't found documentation of r9 in Arduino (assuming Arduino
                // positions the Arduino libraries as a virtual platform), so we'll assume that r9 is designated as v6.
                // Therefore the calling convention requires us to preserve r4-r11 (v1-v8) and sp.

                // AAPCS requires the stack to be word-aligned at all times; furthermore, at public interfaces, the
                // stack must be double-word aligned. While this function appears to be a leaf function, which would
                // nominally mean that we wouldn't be required to maintain double-word alignment, re-entrancy to the
                // thread can be triggered at other points in the code (e.g. when we need to bootstrap a new thread).

                "ldr r1, [r0, #0];" // * (GreenThread**)  [T* and T& both map to the data pointer machine type in AAPCS]

                // The push instruction encodes register_list as 8 1-bit flags, and so can only access the low
                // registers (and optionally LR); mov encodes Rm using 4 bits and can thus access the high registers;
                // therefore, we need to batch our store-multiples in order to push the high registers.
                "push {r4-r7, lr};"
                "mov r2, r8;"
                "mov r3, r9;"
                "mov r4, r10;"
                "mov r5, r11;"
                "push {r2-r5};"

                // align the stack; AAPCS and ARM Thumb C and C++ compilers always use a full descending stack
                "sub sp, #4;"

                // Store the stack pointer into the GreenThread structure's 'sp' member
                "mov r6, sp;"
                "str r6, [r1, #0];"


                // dereference the 'next' pointer in the GreenThread structure
                "ldr r1, [r1, #4];"
                // update the memory that 'current' is aliasing (i.e. g_currentThread) to point to 'next'
                "str r1, [r0, #0];"


                // Load the stack pointer from the GreenThread structure's 'sp' member
                "ldr r6, [r1, #0];"
                "mov r6, sp;"

                // Account for the stack alignment above
                "add sp, #4;"

                // Batch load the registers (see above)
                "pop {r2-r5};"
                "mov r8, r2;"
                "mov r9, r3;"
                "mov r10, r4;"
                "mov r11, r5;"
                "pop {r4-r7, pc};" // pop-and-return
            );
        }

    } // extern "C"



    void yield() {
        switchContext( g_currentThread );
    }

} // namespace


#ifndef COOPMULTITASKING_NO_YIELD_DEF
// hardware/arduino/avr/cores/arduino/hooks.c defines yield() as a weak symbol, which allows us to supply an
// implementation here
extern "C" void yield() {
    CoopMultitasking::yield();
}
#endif
