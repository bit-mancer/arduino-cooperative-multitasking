#ifndef _COOPMULTITASKING_COOPMULTITASKING_H_
#define _COOPMULTITASKING_COOPMULTITASKING_H_

#if ! defined(ARDUINO_ARCH_SAMC) && ! defined(ARDUINO_ARCH_SAMD) && ! defined(ARDUINO_ARCH_SAML)
#error The CoopMultitasking library currently only supports ARM® Cortex®-M0+ processors (e.g. development boards based on Atmel/Microchip SAM C, D, and L MCUs).
#endif

namespace CoopMultitasking {

    using LoopFunc = void (*)();

    extern void yield();
}

#endif
