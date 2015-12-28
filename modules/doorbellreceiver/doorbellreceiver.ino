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
    radio.startListening();;
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
    for (int i = 0; i < 3; i++)
    {
        tone(speakerPin, NOTE_A4, 400);
        delay(400);
        tone(speakerPin, NOTE_F4, 400);

        delay(500);
    }

    noTone(speakerPin);
    digitalWrite(speakerPin, LOW);
}
