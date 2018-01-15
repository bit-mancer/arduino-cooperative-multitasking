# arduino-cooperative-multitasking

[![License][license-image]][license-url]
[![Contributor Code of Conduct][contributing-image]][contributing-url]

CoopMultitasking is a simple cooperative multitasking library for Arduino and Arduino-compatible development boards that use the ARM Cortex-M0 and M0+ processors. See [Compatibility](#compatibility) for a list of supported products.


__Table of Contents__
0. [Getting Started](#getting-started)
0. [Examples](#examples)
0. [Compatibility](#compatibility)
0. [Installation](#installation)
0. [Further Information](#further-information)
0. [Troubleshooting](#troubleshooting)
0. [Implementation Details](#implementation-details)



## Getting Started

After including the library header, call `CoopMultitasking::startLoop()` to run another loop, just like the normal `loop()` you're used to writing in Arduino sketches. The loop function you pass to `CoopMultitasking::startLoop()` will be called immediately.

The new loop will run exclusively until you call `delay()` or `yield()`, at which point the next loop is allowed to run, and so on. Eventually your first loop will become the current loop and get another chance to run.

You're probably used to Arduino's `delay()` function causing the bulk of your code to block, but when using CoopMultitasking `delay()` will only block the loop in which you call it. Another loop gets to run while the first is blocked in `delay()`.



## Examples

> TODO document stack size and/or provide default

Here's a simple example:
```cpp
#include <CoopMultitasking.h>

void setup() {
    CoopMultitasking::startLoop( loop2, 1024 );
}

void loop() {
    Serial.println( "loop" );
    delay( 500 );
}

void loop2() {
    Serial.println( "loop2" );
    delay( 2000 );
}
```

You can start as many loops as you want, constrained only by the memory available on your device:

```cpp
#include <CoopMultitasking.h>

void setup() {
    CoopMultitasking::startLoop( loop2, 1024 );
    CoopMultitasking::startLoop( loop3, 1024 );
}

void loop() {
    Serial.println( "loop" );
    delay( 500 );
}

void loop2() {
    Serial.println( "loop2" );
    delay( 2000 );
}

void loop3() {
    Serial.println( "loop3" );
    delay( 1000 );
}
```

If your existing code is using `millis()` in a tight loop, you'll want to modify the code to also call `yield()` &mdash; this will allow your other loops to run:

```cpp
#include <CoopMultitasking.h>

unsigned long previousMS = 0;
unsigned long intervalMS = 500;

void setup() {
    CoopMultitasking::startLoop( loop2, 1024 );
}

void loop() {

    auto currentMS = millis();

    if ( currentMS - previousMS > intervalMS ) {
        previousMS = currentMS;
        Serial.println( "loop" );
    }

    yield(); // <-- we need to add this to allow loop2 to run!
}

void loop2() {
    Serial.println( "loop2" );
    delay( 2000 ); // it's important to use delay() or yield() in *each* loop!
}
```

__It's important that every loop ultimately calls either `delay()` or `yield()`.__

If you have existing code in a tight loop for a long-running task, you can add `yield()` to allow other loops to run periodically during the task.

```cpp
#include <CoopMultitasking.h>

const int BUTTON_PIN = 2;

void setup() {
    pinMode( BUTTON_PIN, INPUT );

    CoopMultitasking::startLoop( loop2, 1024 );
}

void loop() {
    if ( digitalRead( BUTTON_PIN ) == HIGH ) {
        doLongRunningTask();
    }

    yield(); // it's important to use delay() or yield() in *each* loop!
}

void doLongRunningTask() {
    for ( int i = 0; i < 10000; i++ ) {
        // Yield to other loops every 100 iterations
        if ( i % 100 == 0 ) {
            yield(); // <-- allow loop2 to run during the long-running task!
        }

        // do some work on the long-running task
        // ...
    }
}

void loop2() {
    Serial.println( "loop2" );
    delay( 2000 ); // it's important to use delay() or yield() in *each* loop!
}
```

You don't necessarily need to call `yield()` in a loop-within-a-loop when `yield()` is already being called by the outer loop. In the above example, we have two yields because `loop()` is repeatedly checking for a button press; once it detects the press, it starts a task that takes a long time to complete, so that inner loop also calls `yield()` (because the outer `yield()` won't be called for a long time). If we had a quick task that had a loop instead of a long-running task then we wouldn't need the second `yield().`

In the above example, `yield()` is called every 100 iterations. It's up to you to determine whether yield should be called every iteration or less frequently. Factors influencing the decision would include the amount of time each iteration takes and how often you'd like your other loops to run. If in doubt, call `yield()` on every iteration.



## Compatibility

This library requires a relatively recent version of Arduino IDE (e.g. 1.6+).

This library currently supports ARM Cortex-M0/M0+ processors (e.g. development boards based on Atmel/Microchip SAM C, D, and L MCUs).

__The following products should be compatible:__
* Adafruit Feather M0
* Adafruit Gemma M0
* Adafruit Metro M0
* Adafruit Trinket M0 _(when using Arduino IDE)_
* Arduino M0
* Arduino M0 Pro
* Arduino MKR GSM 1400
* Arduino MKR WAN 1300
* Arduino MKR ZERO
* Arduino MKR1000 WIFI
* Arduino Tian
* Arduino Zero
* _similar Arduino and Arduino-compatible development boards_

Arduino and Arduino-compatible development boards that use the AVR, ESP32, or ESP8266 MCUs are not currently supported.

_CoopMultitasking is a third-party library for Arduino and Arduino-compatible development boards and is not affiliated with Arduino Holding._



## Installation

Download this repository as a zip file and then use the `Sketch->Include Library->Add .ZIP Library` menu in Arduino IDE. See [Installing Additional Arduino Libraries](https://www.arduino.cc/en/Guide/Libraries) for more information.




## Further Information

"Cooperative multitasking" means that there is no scheduler that will suddenly preempt the execution of a thread in order to run another thread; there are no "time slices." Instead, each thread (loop in our case) must periodically and explicitly yield in order to allow other threads to run. All threads must ultimately yield to allow the forward progression of your application across all threads.

This library is currently targeting single-core processors, which means that only one thread (and thus, loop) will execute at any given time.


#### Interrupt handlers

Interrupt handlers will still preempt your code as usual.

__Never__ call `delay()`, `yield()`, or `CoopMultitasking::yield()` from an interrupt handler (which is a good practice anyway: interrupt handlers should be written to take no more time than absolutely necessary).


#### Call order

In CoopMultitasking, a "green thread" runs each loop you start, including the main Arduino `loop()`. When you call `delay()` or `yield()`, the current thread is paused and the next thread is resumed (usually from where it too had been previously paused in `delay()` or `yield()`). To understand the execution flow of your code when using CoopMultitasking, let's consider the first example we saw earlier:

```cpp
#include <CoopMultitasking.h>

void setup() {
    CoopMultitasking::startLoop( loop2, 1024 );
}

void loop() {
    Serial.println( "loop" );
    delay( 500 );
}

void loop2() {
    Serial.println( "loop2" );
    delay( 2000 );
}
```

The call stack for this code can be visualized as follows &mdash; `>>>` indicates that a function is being called, `<<<` indicates that a function is returning to the code that called it, and the dashes (`---`) indicate where one thread is suspended and the next is resumed:

```
1.  >>> setup()
2.      >>> CoopMultitasking::startLoop()
--------------------------------------------
3.          >>> loop2()
4.              [delay is called]
--------------------------------------------
5.      <<< CoopMultitasking::startLoop()
6.  <<< setup()
7.  >>> loop()
8.      [delay is called]
--------------------------------------------
9.          <<< loop2()
10.         >>> loop2()
11.             [delay is called]
--------------------------------------------
12. <<< loop()
13. >>> loop()
14.     [delay is called]
--------------------------------------------
15.        <<< loop2()
16.        >>> loop2()
17.            [delay is called]
--------------------------------------------
18. <<< loop()

...and so on...
```

Notice how the first time `loop2` is called (line 3), it doesn't return immediately; instead, the call to `delay()` causes `loop2` to pause, and the original thread resumes and returns from `CoopMultitasking::startLoop` (line 5). The original thread then completes `setup()` (line 6) and calls into `loop` (line 7). `loop2` gets another chance to run once `loop` calls `delay()` (line 8).

Here is the first example again, with comments on the call order:

```cpp
#include <CoopMultitasking.h>

void setup() {
    // Start loop2 -- it will run immediately, and startLoop() won't return until loop2()
    // calls delay() or yield()
    CoopMultitasking::startLoop( loop2, 1024 );

    // Now that loop2() has yielded (in this case, by calling delay()), execution will
    // continue here.

    // As usual, once setup() returns, the Arduino code will start the main loop().
}

// Your usual Arduino loop() runs as normal; in this example, loop() will run for the
// first time after loop2() has had a chance to run
void loop() {
    Serial.println( "loop" );
    delay( 500 ); // it's important to use delay() or yield() in each loop!
}

// And here's your new loop!
void loop2() {
    Serial.println( "loop2" );
    delay( 2000 ); // it's important to use delay() or yield() in each loop!
}
```



## Troubleshooting

#### Only one loop is running

Make sure _every_ loop eventually calls either `delay()` or `yield()` &mdash; if one of your loops doesn't, then only that loop will get to run and the others will remain paused. It's okay to let your loop run a few times without calling either function, but if these functions are _never_ eventually called, then only that one loop will be able to run. If it looks like you have `delay()` or `yield()` in every loop and you're still getting this problem, then make sure they aren't being skipped by something like an `if` conditional.


#### Example code

If you run the example code exactly as written, you may notice that "loop" is printed _before_ "loop2" in your serial monitor, even though `loop2()` runs before `loop()` &mdash; if you see this, it's because your serial connection has yet to be established the first time `loop2()` runs and prints "loop2". You can add the following code to your setup function to wait for the serial connection before starting your loops:

```cpp
void setup() {
    Serial.begin( 9600 );
    while( ! Serial ); // wait for serial connection to be established

    CoopMultitasking::startLoop( loop2, 1024 );
    // ...
}
```


#### `yield()` is already defined

If you're using another library that also implements Arduino's `::yield()` function, then you can define `COOPMULTITASKING_NO_YIELD_DEF` _before_ including the CoopMultitasking header:

```cpp
#define COOPMULTITASKING_NO_YIELD_DEF
#include <CoopMultitasking.h>
```

This will prevent CoopMultitasking from defining `::yield()`. You can still use this library's yield by calling `CoopMultitasking::yield()`, but keep in mind that the Arduino functions `delay()` and `yield()` will NOT call CoopMultitasking's yield &mdash; they will call the yield defined by the other library you are using. The impact of this is that CoopMultitasking won't be able to switch threads except when `CoopMultitasking::yield()` is explicitly called.


#### Other compiler errors

__This library requires an up-to-date version of Arduino IDE (1.6+).__

If you're using an IDE other than Arduino IDE, and you're getting compilation errors, check if there is a new version of your IDE available that uses an up-to-date version of the build toolchain.

Make sure the library supports your development board &mdash; it has to be a board with an ARM Cortex-M0 or M0+ processor. The library doesn't support classic Arduino development boards that use the AVR microprocessor, or newer boards that use ESP32 or ESP8266 MCUs. See the [Compatibility](#compatibility) section for details.




## Implementation Details

> This section is to document technical decisions in the library and won't impact the majority of users.

#### Thread stack

The stack size you request when calling `CoopMultitasking::startLoop()` will be increased by 36 bytes (to account for thread context information) and will be 8-byte-aligned (which is a processor/call procedure requirement); therefore, your stack may consume 36-43 bytes more than you request. Protected memory, canaries, etc., are not used, and stack overflows will lead to undefined behavior; that is, corruption of adjacent memory directly _below_ the stack (ARM uses a full descending stack).

#### Requirement of each loop to explicitly call `delay()`/`yield()`

The requirement (if you want forward progression across all threads) of calling `delay()` or `yield()` in your loops is for consistency with Arduino's runloop, which doesn't call `yield()` for you in-between iterations:

```cpp
setup();

for (;;) {
    loop();
    if (serialEventRun) serialEventRun();
}
```


[license-image]: https://img.shields.io/badge/license-MIT-blue.svg
[license-url]: LICENSE.md

[contributing-image]: https://img.shields.io/badge/contributing-CoC-blue.svg
[contributing-url]: CONTRIBUTING.md
