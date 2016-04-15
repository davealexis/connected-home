/*
 *  DOORBELL UNIT - NRF24L01+
 *  ---------------------------------------------------------------------------
 *  This version of the doorbell unit uses the NRF24L01+ 2.4GHz radio to send
 *  ring notifications to the base unit.
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

#include <avr/sleep.h>
#include <avr/power.h>
#include <SPI.h>
#include <RF24_config.h>
#include <nRF24L01.h>
#include <RF24.h>

// Define pins used by NRF24L01+ for chip select and chip enable
#define CE_PIN   9
#define CSN_PIN  10

// This is the address of the receiver.
const uint64_t pipe = 0xE8E8F0F0E1LL;

// The message sent to the base unit is an array or two integers - one identifying
// the network (in this case, I used our house number), and another that identifies
// the specific sending unit.  This allows for multiple units to be added to the
// network - e.g. front door, back door, garage, etc.
int message[2];

// Define the NRF24L01+ radio object
RF24 radio(CE_PIN, CSN_PIN);

const int button            = 3;
const int led               = 4;
const int nrfModulePowerPin = 6;
const int debounceDelay     = 500;
const int activityTimeout   = 10000;

volatile bool ringRequested = false;
volatile int lastPressed    = 0;

bool ledOn                  = false;
bool startingUp;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Initialize NRF24L01+ radio to enable communication.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void initializeComm()
{
    // We're powering the radio through digital pin 6 so that we can totally
    // turn off power to it during sleep cycle.
    // Set the pin high to turn on power to the NRF24L01+.
    digitalWrite( nrfModulePowerPin, HIGH );

    // Set up the message that we will be sending to the base station when someone
    // presses the doorbell button.
    message[0] = 517;       // Network ID
    message[1] = 1;         // Unit ID

    // Fire up the layzar!!
    radio.begin();
    radio.openWritingPipe( pipe );

    delay(50);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Flashes the LED, and sends a ring message to the base station.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sendNotify()
{
    radio.write( message, sizeof( message ) );
    flashLed( 1 );
}

/* ------------------- MAIN HANDLERS & LOOP -------------------------------- */

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void flashLed(int repeat)
{
    for (byte i = 0; i < repeat; i++)
    {
        digitalWrite( led, LOW );
        delay( 100 );
        digitalWrite( led, HIGH );
        delay( 100 );
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Handle the button pressed event
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void buttonPressed()
{
    // Handle button debouncing by not handling a button press within 1 second of the previous press.
    if ( !startingUp && !ringRequested && ( millis() - lastPressed > debounceDelay) )
    {
        ringRequested = true;
        ledOn = true;
        lastPressed = millis();
    }

    startingUp = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// The interrupts to wake us up has been triggered by a button press
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void wakeUp()
{
    sleep_disable();

    // When we wake up, we want to trigger the button press event
    // Remove the wakeUp interrupt handler
    detachInterrupt(INT1);

    #ifdef DEBUGMODE
        Serial.println("wake!");
    #endif

//    // Restore AVR peripherals used elsewhere in the code
//    power_usart0_enable(); // Enable Serial
//    power_timer0_enable(); // Enable millis() etc.
//    power_spi_enable();    // Enable SPI

    // Set up button to trigger an interrupt.
    attachInterrupt( INT1, buttonPressed, FALLING );

    delay(50);

    // Turn on the NRF24L01+ radio.
    initializeComm();

    // Simulate button press to set the correct flags
    buttonPressed();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// goToSleep()
// Powers down communication module and microcontroller. The normal button
// interrupt handler is disconnected, and another handler that handles the
// wakup is attached.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void goToSleep(void)
{
    noInterrupts();
    detachInterrupt( INT1 );
    attachInterrupt( INT1, wakeUp, LOW );
    delay(50);
    interrupts();

    // Ensure that the LED is off
    digitalWrite( led, LOW );

    // Disconnect power to the NRF24L01+ radio
    digitalWrite( nrfModulePowerPin, LOW );

    // Initialize sleep mode
    SMCR |= (1 << 2);  // Power down mode
    SMCR |= 1;        // Enable sleep

    // Disable brownout detection
    MCUCR |= (3 << 5);
    MCUCR = (MCUCR & ~(1 << 5)) | (1 << 6);

    // Go to sleep
    __asm__ __volatile("sleep");

    /* The program will continue from here. */
    // ....
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Setup
// Initializes the communication module, and configures the LED and button.
// The button presses are handled with an interrupt handler rather than
// polling.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void setup()
{
    startingUp = true;
    ringRequested = false;

    #ifdef DEBUGMODE
        Serial.begin(115200);
        delay(10);
        Serial.println();
        Serial.println();
        Serial.print("Configuring...");
    #endif

    // Set all pins as output by default.  This reduces power.
    for ( int pin = 1; pin <= 20; pin++ )
    {
        if ( pin != button )
        {
            pinMode( pin, OUTPUT );
            digitalWrite( pin, LOW );
        }
    }


    // Disable the Analog/Digital Converter.  Don't need it.
    ADCSRA &= ~(1 << 7);

    // Now let's set only the button pin differently.
    pinMode( button, INPUT_PULLUP );

    // Set up the NRF24L01+ radio to be ready to communicate
    initializeComm();

    // Flash things to let the doorbell user know that something is going on
    flashLed(2);
    ledOn = true;

    // Set up button to trigger an interrupt.
    noInterrupts();
    attachInterrupt( INT1, buttonPressed, FALLING );
    interrupts();
    delay(50);

    startingUp = false;

    #ifdef DEBUGMODE
        Serial.println();
        Serial.print("Done.");
    #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Main loop
// Sends the wireless notification to the base unit if someone presses the
// doorbell.  Flashes the LED then leaves it solid on for 20 seconds.  If
// the bell is not pressed again in that time, the device is put to sleep.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void loop()
{
    if ( ringRequested )
    {
        #ifdef DEBUGMODE
            Serial.println( "Button pressed" );
            Serial.println( "Sending message to base station ..." );
        #endif

        sendNotify();

        // Reset the ring request flag so button presses are allowed again.
        ringRequested = false;
    }

    // Let's check if the unit has been awake longer than the activity timeout
    // since the last time the button was pressed.  If so, put the unit to sleep.

    int timePassed = millis() - lastPressed;

    if ( timePassed > activityTimeout )
    {
        flashLed(2);
        digitalWrite( led, LOW );
        ledOn = false;

        goToSleep();
    }
    else
    {
        // When the button is pressed, flash the LED every half a second for 2 seconds
        // to let the user know that something is happening.
        if ( timePassed < 1500 && (0 < (timePassed % 500) < 10) )
        {
            flashLed(1);
        }
    }
}
