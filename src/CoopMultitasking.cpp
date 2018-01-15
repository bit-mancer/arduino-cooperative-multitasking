#include <cstdint>
#include <malloc.h>


#include "CoopMultitasking.h"

#define _COOPMULTITASKING_CORTEXM_WORD_SIZE 4
#define _COOPMULTITASKING_CORTEXM_THREAD_CONTEXT_WORDS 9

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
    static uint32_t g_threadID = 0;

    class Init final {
    public:
        Init() {
            g_currentThread = new GreenThread;
            g_currentThread->id = g_threadID++;
            g_currentThread->next = g_currentThread;
            // The remaining members are set during the first context switch
        }
    };

    static Init g_init;




    extern "C" {

        static void __attribute__((noinline)) __attribute__((used)) runLoop( LoopFunc func ) {
            while ( true ) {
                func();
            }
        }


        /**
         * Bootstraps a new thread for execution, and then performs a context switch from the current thread to the new
         * thread.
         *
         * @param currentThread A reference to the currently-executing thread. The reference will be updated to point
         *                      to the new thread. Typically you would pass a reference to the global current thread
         *                      instance.
         * @param newThread The new thread to bootstrap; the new thread should already have a stack allocated that is
         *                  double-word aligned.
         */
        static void __attribute__((naked)) __attribute__((noinline)) bootstrapAndSwitchToNewThread(
                    GreenThread* currentThread,
                    GreenThread* newThread,
                    LoopFunc func ) {

            // compiler hints for unused parameter warnings
            (void)currentThread;
            (void)newThread;
            (void)func;

            asm (

                // AAPCS states that a subroutine must preserve the contents of the registers r4-r8, r10, r11 and sp
                // (and r9 in PCS variants that designate r9 as v6). Register 9 is the platform register: its meaning
                // is defined by the virtual platform. I haven't found documentation of r9 in Arduino (assuming Arduino
                // positions the Arduino libraries as a virtual platform), so we'll assume that r9 is designated as v6.
                // Therefore the calling convention requires us to preserve r4-r11 (v1-v8) and sp.

                // We store 9 words of context information on the stack while suspending a thread. AAPCS requires the
                // stack to be word-aligned at all times; furthermore, at public interfaces, the stack must be
                // double-word aligned. While re-entrancy to the thread can occur at other points in the code (e.g.
                // when we need to perform a context switch), these are all internal functions and the act of restoring
                // the thread context will re-align the stack to a double-word.

                // The push instruction encodes register_list as 8 1-bit flags, and so can only access the low
                // registers (and optionally LR); mov encodes Rm using 4 bits and can thus access the high registers;
                // therefore, we need to batch our store-multiples in order to push the high registers.
                "push {r4-r7, lr};"
                "mov r4, r8;"
                "mov r5, r9;"
                "mov r6, r10;"
                "mov r7, r11;"
                "push {r4-r7};"

                // Store the stack pointer into the current GreenThread's 'sp' member
                "mov r7, sp;"
                "str r7, [r0, #0];"


                // bootstrap the new thread

                // Load the stack pointer from the next GreenThread's 'sp' member; the stack is already
                // double-word-aligned (see startLoop())
                "ldr r4, [r1, #0];"
                "mov sp, r4;"

                // Start the run loop
                "mov r0, r2;"
                "bl runLoop;"
            );
        }


        /**
         * Switch the thread context from the provided current thread, to the provided next thread.
         *
         * @param current The currently-executing thread. The reference will be updated to point to the
         *                next thread. Typically you would pass a reference to the global current thread instance.
         */
        static void __attribute__((naked)) __attribute__((noinline)) switchContext(
                GreenThread* current,
                GreenThread* next ) {

            // compiler hints for unused parameter warnings
            (void)current;
            (void)next;

            asm (
                // AAPCS states that a subroutine must preserve the contents of the registers r4-r8, r10, r11 and sp
                // (and r9 in PCS variants that designate r9 as v6). Register 9 is the platform register: its meaning
                // is defined by the virtual platform. I haven't found documentation of r9 in Arduino (assuming Arduino
                // positions the Arduino libraries as a virtual platform), so we'll assume that r9 is designated as v6.
                // Therefore the calling convention requires us to preserve r4-r11 (v1-v8) and sp.

                // We store 9 words of context information on the stack while suspending a thread. AAPCS requires the
                // stack to be word-aligned at all times; furthermore, at public interfaces, the stack must be
                // double-word aligned. While re-entrancy to the thread can occur at other points in the code (e.g.
                // when we need to bootstrap a new thread), these are all internal functions and the act of restoring
                // the thread context will re-align the stack to a double-word.

                // The push instruction encodes register_list as 8 1-bit flags, and so can only access the low
                // registers (and optionally LR); mov encodes Rm using 4 bits and can thus access the high registers;
                // therefore, we need to batch our store-multiples in order to push the high registers.
                "push {r4-r7, lr};"
                "mov r2, r8;"
                "mov r3, r9;"
                "mov r4, r10;"
                "mov r5, r11;"
                "push {r2-r5};"

                // Store the stack pointer into the current GreenThread's 'sp' member
                "mov r6, sp;"
                "str r6, [r0, #0];"


                // Load the stack pointer from the next GreenThread's 'sp' member
                "ldr r6, [r1, #0];"
                "mov sp, r6;"

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
        auto current = g_currentThread;
        g_currentThread = current->next;
        switchContext( current, g_currentThread );
    }


    void startLoop( LoopFunc func, uint32_t stackSize ) {

        auto newThread = new GreenThread;

        newThread->id = g_threadID++;

        // Account for the thread context information that will be pushed onto the stack during a context switch, so
        // that the requested stack size is fully-available
        stackSize += (_COOPMULTITASKING_CORTEXM_THREAD_CONTEXT_WORDS * _COOPMULTITASKING_CORTEXM_WORD_SIZE);

        // AAPCS requires that the stack be word-aligned at all times; furthermore, at public interfaces, the stack
        // must be double-word aligned.
        auto stackExtent = stackSize + (stackSize % 8);
        auto stack = (uint32_t) ::memalign( 8, stackExtent );

        // AAPCS and ARM Thumb C and C++ compilers always use a full descending stack
        newThread->sp = stack + stackExtent;

        // NOTE: at this time, this library is loop-only and therefore doesn't allow threads to be stopped, so we'll
        // skip storing the allocated pointer since we never need to free it

        newThread->next = g_currentThread->next;
        g_currentThread->next = newThread;

        auto current = g_currentThread;
        g_currentThread = newThread;

        bootstrapAndSwitchToNewThread( current, newThread, func );

        // The current thread will resume execution here after the new thread performs its first context switch
    }

} // namespace


#ifndef COOPMULTITASKING_NO_YIELD_DEF
// hardware/arduino/avr/cores/arduino/hooks.c defines yield() as a weak symbol, which allows us to supply an
// implementation here
extern "C" void yield() {
    CoopMultitasking::yield();
}
#endif
