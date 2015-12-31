/*
 *  DOORBELL RECEIVER UNIT - NRF24L01+
 *  ---------------------------------------------------------------------------
 *  This version of the doorbell unit uses the NRF24L01+ 2.4GHz radio to receive
 *  ring notifications from the doorbell unit.
 *  This implementation is the first version of the doorbell, with limitted
 *  functionality - i.e. Just a doorbell.  The initial vision was for a full
 *  door monitoring system with a motion sensor to wake the device, camera and
 *  audio interaction with the person at the door, a weather station capturing
 *  temperature, humidity, and barometric pressure.
 *
 *  ATTRIBUTIONS
 *
 *  Uses the optimized RF24 library from TMRh20. https://github.com/TMRh20/RF24
 *  ---------------------------------------------------------------------------
 */

#include <SPI.h>
#include <RF24_config.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "notes.h"

#define CE_PIN  6
#define CSN_PIN 8

#define speakerPin 5

const uint64_t pipe = 0xE8E8F0F0E1LL;

int message[2];

RF24 radio(CE_PIN, CSN_PIN); // Create a Radio

void setup()
{
    pinMode( speakerPin, OUTPUT );

    radio.begin();
    radio.openReadingPipe(1, pipe);
    radio.startListening();
}

void loop()
{
    if ( radio.available() )
    {
        // Read the data payload until we've received everything
        //bool done = false;

        while ( radio.available() )
        {
            radio.read( &message, sizeof(message) );
        }

        if ( message[0] == 517 )
        {
            ringBell();
        }
    }
}

void ringBell()
{
    tone(speakerPin, NOTE_A5, 800);
    delay(800);
    tone(speakerPin, NOTE_F5, 800);
    delay(800);

    noTone(speakerPin);
    digitalWrite(speakerPin, LOW);
}
