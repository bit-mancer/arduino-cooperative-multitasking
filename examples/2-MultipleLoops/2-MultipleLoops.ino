// See https://github.com/bit-mancer/arduino-cooperative-multitasking for complete documentation.

// You can start several loops, constrained only by the memory available on your device:

#include <CoopMultitasking.h>

void setup() {
    // wait for serial connection to be established
    // (this isn't required by CoopMultitasking, but it makes the output in your Serial Monitor consistent)
    Serial.begin( 9600 );
    while( ! Serial );

    /**
     * Typically you would start a handul of additional loops in `setup()`. Each loop you start will consume a fixed
     * amount of memory which will limit the total number of loops you can start on your device. If you're starting a
     * lot of loops, you'll have to make sure you leave enough free memory available for the rest of your application;
     * see the "Stack Size" section of README.md for more information on memory usage.
     */

    CoopMultitasking::startLoop( loop2 );
    CoopMultitasking::startLoop( loop3 );
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
