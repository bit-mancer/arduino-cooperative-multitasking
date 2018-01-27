#include <cstdint>
#include <malloc.h>

// The library implementation files do not include the API (root) CoopMultitasking.h header in order to allow sketches
// to #define COOPMULTITASKING_NO_YIELD_DEFINE prior to including the API header. Instead, headers that are common
// between the implementation and the API header are used (such as Types.h).
#include "CoopMultitasking/Types.h"

#define COOPMULTITASKING_CORTEXM_WORD_SIZE 4
#define COOPMULTITASKING_CORTEXM_FIBER_CONTEXT_WORDS 9 // 8 (base registers) + 1 (for bootstrapping)

namespace CoopMultitasking {

    /**
     * Holds partial context information for a fiber; the remainder of the context is stored on the fiber's stack.
     *
     * NOTE: assembly code expects the initial part of this structure to be well-defined.
     */
    struct Fiber final {

        // BEGIN assembly code expects this ordering
        uint32_t sp;
        uint32_t pc;
        // END assembly code expects this ordering

        Fiber* next;
    };


    static Fiber* g_currentFiber = nullptr;

    class Init final {
    public:
        Init() {
            g_currentFiber = new Fiber;
            g_currentFiber->next = g_currentFiber;
            // The remaining members are set during bootstrapping and the first context switch
        }
    };

    static Init g_init;




    extern "C" {

        /**
         * Runs an Arduino-style loop function.
         */
        static void __attribute__((noinline)) __attribute__((used)) runLoop( LoopFunc func ) {
            while ( true ) {
                func();
            }
        }


        /**
         * Acts as a trampoline between 'runLoop' and an assembly caller that needs to pass the loop function on the
         * stack.
         *
         * IMPORTANT: This function has the following private contract: the single parameter (the loop function) is
         * passed on the stack.
         *
         * @param (a word passed on the stack) The loop function address.
         */
        static void __attribute__((naked)) __attribute__((noinline)) __attribute__((used)) runLoopTrampoline() {
            asm (
                // AAPCS requires the stack to be word-aligned at all times; furthermore, at public interfaces,
                // the stack must be double-word aligned. Popping the bootstrapped stack will restore double-word
                // alignment.

                "pop {r0};"
                "bl runLoop;"
            );
        }


        /**
         * Returns true if the processor is in Thread mode (IPSR Exception Number is 0), or false if the processor is
         * in Handler mode (IPSR Exception Number is non-zero).
         */
        static bool __attribute__((naked)) __attribute__((noinline)) isProcessorInThreadMode() {
            asm (
                // select the exception number from the IPSR
                //  [31:6] Reserved
                //  [5:0] Exception Number

                // if we're in Thread mode, the IPSR exception number will be 0

                "mrs r0, ipsr;"

                // select the exception number
                "mov r3, #63;" // 63 == 0x3F == 0011 1111
                "and r0, r3;"

                // TODO notes for future support of the Cortex-M line: the 'S' variants of the following instructions
                // are Thumb-2 and are not supported on the M0 (the instructions always set the flags in Thumb-1).

                // Negate the exception number value; the carry flag is set to 0 if the subtraction produces a borrow,
                // and to 1 otherwise (so a zero value will have carry 1, and a non-zero value will have carry 0)
                "neg r3, r0;" // (result, carry, overflow) = AddWithCarry( NOT(r0), 0, '1' )

                // Add the value, negative value, and the carry flag together (thus a zero exception number will be 1,
                // and a non-zero will be 0)
                "adc r0, r3;" // effectively (result, carry, overflow) = AddWithCarry( r0, r3, APSR.C )

                "bx lr;"
            );
        }


        /**
         * Bootstraps a new fiber for execution; a context switch is NOT performed.
         *
         * This function operates only on the provided parameters and doesn't update any globals.
         *
         * @param newFiber The new fiber to bootstrap; the new fiber should already have a stack allocated that is
         *                  double-word aligned.
         * @param func The function that will be called repeatedly via runLoop().
         */
        static void __attribute__((naked)) __attribute__((noinline)) bootstrapNewFiber(
                    Fiber* newFiber,
                    LoopFunc func ) {

            // compiler hints for unused parameter warnings
            (void)newFiber;
            (void)func;

            asm (
                // AAPCS requires the stack to be word-aligned at all times; furthermore, at public interfaces, the
                // stack must be double-word aligned. However this function and the bootstrap trampoline are coupled
                // internal functions; the trampoline will restore double-word alignment prior to further procedure
                // calls.

                // Temporarily swap stack pointers so that we can bootstrap
                "mov r2, sp;"
                "ldr r3, [r0, #0];" // load the stack pointer from the Fiber's 'sp' member
                "mov sp, r3;"

                // runLoopTrampoline() has a private contract: the single parameter (the loop function) is passed on
                // the stack. The stack will be popped and double-word alignment restored by the trampoline.
                "push {r1};"

                // Set up the appropriate number of registers on the stack so that we don't pop above the stack when
                // the fiber is first yielded to. The values of the registers are irrelevant.
                "push {r4-r7};"
                "push {r4-r7};"

                // We changed the stack pointer so we need to update the 'sp' member of the Fiber structure
                "mov r3, sp;"
                "str r3, [r0, #0];"

                // Restore the stack pointer
                "mov sp, r2;"

                // Bounce the call to runLoop through runLoopTrampoline (store the address of runLoopTrampoline into
                // the new Fiber's 'pc' member)
                "ldr r3, =runLoopTrampoline;"
                "str r3, [r0, #4];"

                "bx lr;"
            );
        }


        /**
         * Switch the context from the provided current fiber to the provided next fiber.
         *
         * This function operates only on the provided parameters and doesn't update any globals.
         *
         * @param currentFiber The currently-executing fiber. Typically you would pass the global current fiber
         *                      instance.
         * @param next The fiber to switch to.
         */
        static void __attribute__((naked)) __attribute__((noinline)) switchContext( Fiber* current, Fiber* next ) {

            // compiler hints for unused parameter warnings
            (void)current;
            (void)next;

            asm (
                // AAPCS states that a subroutine must preserve the contents of the registers r4-r8, r10, r11 and sp
                // (and r9 in PCS variants that designate r9 as v6). Register 9 is the platform register: its meaning
                // is defined by the virtual platform. I haven't found documentation of r9 in Arduino (assuming Arduino
                // positions the Arduino libraries as a virtual platform), so we'll assume that r9 is designated as v6.
                // Therefore the calling convention requires us to preserve r4-r11 (v1-v8) and sp.

                // We store 9 words of context information on the stack while suspending a fiber. AAPCS requires the
                // stack to be word-aligned at all times; furthermore, at public interfaces, the stack must be
                // double-word aligned. While re-entrancy to the fiber can occur at other points in the code (e.g.
                // when we need to bootstrap a new fiber), these are all internal functions and the act of restoring
                // the fiber context will re-align the stack to a double-word.

                // The ARMv6-M push instruction encodes register_list as 8 1-bit flags, and so can only access the low
                // registers (and optionally LR); mov encodes Rm using 4 bits and can thus access the high registers;
                // therefore, we need to batch our store-multiples in order to push the high registers.
                "push {r4-r7};"
                "mov r2, r8;"
                "mov r3, r9;"
                "mov r4, r10;"
                "mov r5, r11;"
                "push {r2-r5};"

                // Store the stack pointer into the current Fiber's 'sp' member
                "mov r6, sp;"
                "str r6, [r0, #0];"

                // Store the return address into the current Fiber's 'pc' member
                "mov r7, lr;"
                "str r7, [r0, #4];"


                // Load the stack pointer from the next Fiber's 'sp' member
                "ldr r6, [r1, #0];"
                "mov sp, r6;"

                // Batch load the registers (see above)
                "pop {r2-r5};"
                "mov r8, r2;"
                "mov r9, r3;"
                "mov r10, r4;"
                "mov r11, r5;"
                "pop {r4-r7};"

                // Load the return address from the next Fiber's 'pc' member
                "ldr r2, [r1, #4];"
                "mov pc, r2;"
            );
        }

    } // extern "C"



    void yield() {

        if ( ! isProcessorInThreadMode() ) {
            // ignore calls from Handler mode (we can't safely do a context switch while in an exception handler
            // because the execution state back in Thread mode is unknown)
            return;
        }

        auto current = g_currentFiber;
        g_currentFiber = current->next;
        switchContext( current, g_currentFiber );
    }


    Result startLoop( LoopFunc func, uint32_t stackSize ) {

        if ( ! isProcessorInThreadMode() ) {
            // ignore calls from Handler mode (we can't safely use memalign or do a context switch while in an
            // exception handler)
            return Result::NotAllowed;
        }

        auto newFiber = new Fiber;

        // Account for the context information that will be pushed onto the stack during a context switch, so that the
        // requested stack size is fully-available
        stackSize += (COOPMULTITASKING_CORTEXM_FIBER_CONTEXT_WORDS * COOPMULTITASKING_CORTEXM_WORD_SIZE);

        // AAPCS requires that the stack be word-aligned at all times; furthermore, at public interfaces, the stack
        // must be double-word aligned.
        auto stackExtent = stackSize + (stackSize % 8);
        auto pStack = ::memalign( 8, stackExtent );

        if ( pStack == nullptr ) {
            delete newFiber;
            return Result::OutOfMemory;
        }

        // AAPCS and ARM Thumb C and C++ compilers always use a full descending stack
        newFiber->sp = (uint32_t)pStack + stackExtent;

        // NOTE: at this time, this library is loop-only and therefore doesn't allow fibers to be stopped, so we'll
        // skip storing the allocated pointer since we never need to free it

        bootstrapNewFiber( newFiber, func );


        newFiber->next = g_currentFiber->next;
        g_currentFiber->next = newFiber;

        auto current = g_currentFiber;
        g_currentFiber = newFiber;

        switchContext( current, g_currentFiber );

        return Result::Success;
    }

} // namespace
