// See https://github.com/bit-mancer/arduino-cooperative-multitasking for complete documentation.

// If you have existing code in a tight loop for a long-running task, you can add `yield()` to allow other loops to run
// periodically during the task.

#include <CoopMultitasking.h>

// IMPORTANT: If you want to run this example on your device, you'll need to check your board's pinout, connect a
// button, and update this file to provide the correct pin for the button:
const int BUTTON_PIN = 2;

void setup() {
    // wait for serial connection to be established
    // (this isn't required by CoopMultitasking, but it makes the output in your Serial Monitor consistent)
    Serial.begin( 9600 );
    while( ! Serial );


    pinMode( BUTTON_PIN, INPUT_PULLUP );

    CoopMultitasking::startLoop( loop2 );
}

void loop() {
    if ( digitalRead( BUTTON_PIN ) == LOW ) {
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

    Serial.println( "Task complete!" );
}

void loop2() {
    Serial.println( "This is our new loop!" );
    delay( 2000 ); // it's important to use delay() or yield() in *each* loop!
}

/**
 * You don't necessarily need to call `yield()` in a loop-within-a-loop when `yield()` is already being called by the
 * outer loop. In this example we have two yields because the inner loop takes a long time to complete; if it was a
 * quick task or fast loop then it would be fine to only have a yield on the outer loop.
 */
