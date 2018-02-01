// See https://github.com/bit-mancer/arduino-cooperative-multitasking for complete documentation.

/**
 * If your existing code is using `millis()` in a tight loop, you'll want to modify the code to also call `yield()` --
 * this will allow your other loops to run.
 *
 * It's important that every loop ultimately calls either `delay()` or `yield()`.
 *
 * If you're unfamiliar with `yield()`, see the Timing section of README.md.
 */

#include <CoopMultitasking.h>

unsigned long previousMS = 0;
unsigned long intervalMS = 500;

void setup() {
    // wait for serial connection to be established
    // (this isn't required by CoopMultitasking, but it makes the output in your Serial Monitor consistent)
    Serial.begin( 9600 );
    while( ! Serial );

    CoopMultitasking::startLoop( loop2 );
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
