// See https://github.com/bit-mancer/arduino-cooperative-multitasking for complete documentation.

// Here's a simple example:

#include <CoopMultitasking.h>

void setup() {
    // wait for serial connection to be established
    // (this isn't required by CoopMultitasking, but it makes the output in your Serial Monitor consistent)
    Serial.begin( 9600 );
    while( ! Serial );

    // Typically you would start your new loops in setup():
    CoopMultitasking::startLoop( loop2 );
}

void loop() {
    Serial.println( "This is the normal Arduino loop." );
    delay( 500 );
}

void loop2() {
    Serial.println( "This is our new loop!" );
    delay( 2000 );
}
