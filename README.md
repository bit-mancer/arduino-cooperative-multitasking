# arduino-cooperative-multitasking

[![License][license-image]][license-url]
[![Contributor Code of Conduct][contributing-image]][contributing-url]

CoopMultitasking is a simple cooperative multitasking library for Arduino and Arduino-compatible development boards that use the ARM Cortex-M0 and M0+ processors (see [Compatibility](#compatibility) for a list of supported products).


__Table of Contents__
* [Getting Started](#getting-started)
* [Examples](#examples)
* [Compatibility](#compatibility)
* [Installation](#installation)
* [Further Information](#further-information)
* [Troubleshooting](#troubleshooting)
* [Implementation Details](#implementation-details)



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
    Serial.println( "This is the normal Arduino loop." );
    delay( 500 );
}

void loop2() {
    Serial.println( "This is our new loop!" );
    delay( 2000 );
}
```


<br>
You can start as many loops as you want, constrained only by the memory available on your device:

```cpp
#include <CoopMultitasking.h>

void setup() {
    CoopMultitasking::startLoop( loop2, 1024 );
    CoopMultitasking::startLoop( loop3, 1024 );
}

void loop() {
    Serial.println( "This is the normal Arduino loop." );
    delay( 500 );
}

void loop2() {
    Serial.println( "This is our new loop!" );
    delay( 2000 );
}

void loop3() {
    Serial.println( "This is our third loop!" );
    delay( 1000 );
}
```
> TODO link to stack size notes


<br>
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
        Serial.println( "This is the normal Arduino loop." );
    }

    yield(); // <-- we need to add this to allow loop2 to run!
}

void loop2() {
    Serial.println( "This is our new loop!" );
    delay( 2000 ); // it's important to use delay() or yield() in *each* loop!
}
```

__It's important that every loop ultimately calls either `delay()` or `yield()`.__

_If you're unfamiliar with `yield()`, see the [Timing](#timing) section._


<br>
If you have existing code in a tight loop for a long-running task, you can add `yield()` to allow other loops to run periodically during the task.

```cpp
#include <CoopMultitasking.h>

const int BUTTON_PIN = 2;

void setup() {
    pinMode( BUTTON_PIN, INPUT_PULLUP );

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
        // do some work on the long-running task
        // ...
        yield(); // <-- allow loop2 to run during the long-running task!
    }
}

void loop2() {
    Serial.println( "This is our new loop!" );
    delay( 2000 ); // it's important to use delay() or yield() in *each* loop!
}
```

You don't necessarily need to call `yield()` in a loop-within-a-loop when `yield()` is already being called by the outer loop. In the above example we have two yields because the inner loop takes a long time to complete; if it was a quick task or fast loop then it would be fine to only have a yield on the outer loop.



## Compatibility

This library requires a relatively recent version of Arduino IDE (e.g. 1.6+).

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

This library currently supports ARM Cortex-M0/M0+ processors (e.g. development boards based on Atmel/Microchip SAM C, D, and L MCUs). Arduino and Arduino-compatible development boards that use the AVR, ESP32, or ESP8266 MCUs are not currently supported.

_CoopMultitasking is a third-party library for Arduino and Arduino-compatible development boards and is not affiliated with Arduino Holding._



## Installation

Download this repository as a zip file and then use the `Sketch->Include Library->Add .ZIP Library` menu in Arduino IDE. See [Installing Additional Arduino Libraries](https://www.arduino.cc/en/Guide/Libraries) for more information.




## Further Information

"Cooperative multitasking" means that there is no scheduler that will suddenly preempt the execution of a thread in order to run another thread; there are no "time slices." Instead, each of your loops is run by a "fiber" which you can think of as a lightweight thread that stores information about which piece of your code it is running. Different fibers will be running different parts of your code.

One fiber (the "main" or "original" fiber) will run the Arduino `loop()` function, and each new loop you create will be run by a new fiber; if you create two new loops then you will have three fibers in total.

Each fiber (and thus loop you write) must periodically and explicitly yield in order to allow other fibers to run. All fibers must ultimately yield to allow the forward progression of your application. See the [Examples](#examples) for what it means to "yield," and see [Call order](#call-order) for a more detailed description of how fibers run your loops.

This library is targeting single-core processors, which means that only one fiber (and thus, loop) can execute at any given time.


#### Timing

Once you call `delay()` or `yield()`, other loops are allowed to run; once they have also called `delay()` or `yield()`, the first loop is allowed to run again (the loops run in a round-robin fashion). The amount of time the first loop has to wait before it gets another chance to run depends on how many loops you have and how long they run before they call `delay()` or `yield()`.

You're probably familiar with `delay()`, but `yield()` might be something you haven't used before. Arduino's `delay()` function actually calls `yield()` periodically (about once a millisecond), and it is `yield()` itself that allows other loops to run. To allow all loops to run as often as possible you will want to make sure that nothing ties up the processor without calling `yield()`. For example, you'll generally want to avoid any long-running `for` or `while` loops such as this:

```cpp
void loop2() {
    for ( int i = 0; i < 1000000; i++ ) {
        // do some work that takes a while
    }

    delay( 1000 );
}
```

In most circumstances you'll want to modify code like that shown above to add a `yield()` in the middle of your long-running tasks:

```cpp
void loop2() {
    for ( int i = 0; i < 1000000; i++ ) {
        // do some work that takes a while
        yield(); // <-- give other loops a chance to run
    }

    delay( 1000 );
}
```

<br>
There is a time-cost to switching between loops. Most projects won't need to worry about this, but if you have a lot of loops then you may not want to call `yield()` on every iteration. For example if you have a for-loop with a high iteration count but each iteration takes very little time to run, then it might be better to call `yield()` every few iterations:

```cpp
void doLongRunningTask() {
    for ( int i = 0; i < 10000; i++ ) {
        // do some quick work on the long-running task
        // ...

        // Yield to other loops every 100 iterations
        if ( i % 100 == 0 ) {
            yield();
        }
    }
}
```


#### Interrupt handlers

_You can safely skip this section if you don't use interrupts._

Interrupt handlers &mdash; the functions you pass to `attachInterrupt()` &mdash; will still preempt your code as usual.

If you have written interrupt handlers that use shared global variables then you have experience in marking those variables as `volatile`, which makes it safe to use those variables both from interrupt handlers and normal code. You __don't__ need to mark variables shared by CoopMultitasking fibers (loops) as `volatile` __unless__ those variables are also used by interrupt handlers.

Calls to `yield()` (and thus `CoopMultitasking::yield()`) and `CoopMultitasking::startLoop()` from an interrupt handler are __ignored__; calls to `delay()` from an interrupt handler will block, but not switch fibers. As a general practice, whether you use CoopMultitasking or not, you shouldn't call `delay()` or `yield()` in an interrupt handler &mdash; handlers should be as small and fast as possible.

If you'd like to start a loop from an interrupt handler, you should instead start the loop in `setup()` and then notify the loop that the interrupt has occurred:

```cpp
#define BUTTON_A 9

volatile bool buttonTriggered = false;

void setup() {
    pinMode( BUTTON_A, INPUT_PULLUP );
    attachInterrupt( digitalPinToInterrupt( BUTTON_A ), isrButton, FALLING );

    CoopMultitasking::startLoop( triggeredLoop, 1024 );
}

void loop() {
    // do whatever else your project needs to do...
    yield();
}

void triggeredLoop() {
    if ( buttonTriggered ) {
        buttonTriggered = false;

        // handle the button event
    }

    yield();
}

void isrButton() {
    buttonTriggered = true;
}
```


#### Handling error conditions

`CoopMultitasking::startLoop()` returns a value indicating whether the loop was started. You can take a look at [CoopMultitasking.h](#src/CoopMultitasking.h) to see all of the possible values, but the out-of-memory condition is the one you likely want to check for:

```cpp
if ( CoopMultitasking::startLoop( loop2, 1024 )) == CoopMultitasking::Result::OutOfMemory ) {
    // (do whatever is appropriate for your project)
    Serial.println( "Failed to start loop: OUT OF MEMORY!" );
}
```


#### Call order

In CoopMultitasking, a "fiber" runs each loop you start, including the main Arduino `loop()`. When you call `delay()` or `yield()`, the current fiber is paused and the next fiber is resumed (usually from where it too had been previously paused in `delay()` or `yield()`). To understand the execution flow of your code when using CoopMultitasking, let's consider the first example we saw earlier:

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

The call stack for this code can be visualized as follows:

(`>>>` indicates that a function is being called, `<<<` indicates that a function is returning to the code that called it, and the dashes (`---`) indicate where one fiber is suspended and the next is resumed)

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

Notice how the first time `loop2` is called (line 3), it doesn't return immediately; instead, the call to `delay()` causes `loop2` to pause, and the original fiber resumes and returns from `CoopMultitasking::startLoop` (line 5). The original fiber then completes `setup()` (line 6) and calls into `loop` (line 7). `loop2` gets another chance to run once `loop` calls `delay()` (line 8).

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

If you aren't familiar with Arduino libraries, check out the tutorials that Arduino and Adafruit have put together:

<https://www.arduino.cc/en/Guide/Libraries>

<https://learn.adafruit.com/adafruit-all-about-arduino-libraries-install-use/>


#### Only one loop is running

Make sure _every_ loop eventually calls either `delay()` or `yield()` &mdash; if one of your loops doesn't, then only that loop will get to run and the others will remain paused. It's okay to let your loop run a few times without calling either function, but if these functions are _never_ eventually called, then only that one loop will be able to run. If it looks like you have `delay()` or `yield()` in every loop and you're still getting this problem, then make sure they aren't being skipped by something like an `if` statement.


#### Example code

If you run the example code exactly as written, you may notice that `This is the normal Arduino loop.` is printed _before_ `This is our new loop!` in your serial monitor, even though `loop2()` runs before `loop()` &mdash; if you see this, it's because your serial connection has yet to be established the first time `loop2()` runs and calls `Serial.println()`. You can add the following code to your setup function to wait for the serial connection before starting your loops:

```cpp
void setup() {
    Serial.begin( 9600 );
    while( ! Serial ); // wait for serial connection to be established

    CoopMultitasking::startLoop( loop2, 1024 );
    // ...
}
```


#### `yield()` is already defined

If you're using another library that also implements Arduino's `::yield()` function, then you can define `COOPMULTITASKING_NO_YIELD_DEFINE` _before_ including the CoopMultitasking header:

```cpp
#define COOPMULTITASKING_NO_YIELD_DEFINE
#include <CoopMultitasking.h>
```

This will prevent CoopMultitasking from defining `::yield()`. You can still use this library's yield by calling `CoopMultitasking::yield()`, but keep in mind that the Arduino functions `delay()` and `yield()` will NOT call CoopMultitasking's yield &mdash; they will call the yield defined by the other library you are using. The impact of this is that CoopMultitasking won't be able to switch fibers except when `CoopMultitasking::yield()` is explicitly called.


#### Other compiler errors

__This library requires an up-to-date version of Arduino IDE (1.6+).__

If you're using an IDE other than Arduino IDE, and you're getting compilation errors, check if there is a new version of your IDE available that uses an up-to-date version of the build toolchain.

Make sure the library supports your development board &mdash; it has to be a board with an ARM Cortex-M0 or M0+ processor. The library doesn't support classic Arduino development boards that use the AVR microprocessor, or newer boards that use ESP32 or ESP8266 MCUs. See the [Compatibility](#compatibility) section for details.




## Implementation Details

> This section is to document technical decisions in the library and won't impact the majority of users.

#### Stack size

The stack size you request when calling `CoopMultitasking::startLoop()` will be increased by 36 bytes (to account for context information) and will be 8-byte-aligned (which is a processor/call procedure requirement); therefore, your stack may consume 36-43 more bytes than you request. Protected memory, canaries, etc., are not used, and stack overflows will lead to undefined behavior; that is, corruption of adjacent memory directly _below_ the stack (ARM uses a full descending stack).

#### Requirement of each loop to explicitly call `delay()`/`yield()`

The requirement (if you want forward progression across all fibers) of calling `delay()` or `yield()` in your loops is for consistency with Arduino's runloop, which doesn't call `yield()` in-between iterations:

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
