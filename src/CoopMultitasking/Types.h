#ifndef COOPMULTITASKING_COOPMULTITASKING_TYPES_H_
#define COOPMULTITASKING_COOPMULTITASKING_TYPES_H_

namespace CoopMultitasking {

    using LoopFunc = void (*)();


    enum class Result {

        /**
         * No error; the operation was successful.
         */
        Success,

        /**
         * Failed to allocate memory on the heap.
         */
        OutOfMemory,

        /**
         * The requested operation was not allowed in the current context. Consult the function documentation for more
         * information.
         */
        NotAllowed
    };
}

#endif // header guard
