/*
 *  ---------------------------------------------------------------------------
 *
 *
 *  ---------------------------------------------------------------------------
 */

#include <avr/sleep.h>
#include <avr/power.h>
#include <ESP8266WiFi.h>

const char *ssid = "<ssid>";
const char *password = "<network password>";
const char* host = "192.168.1.9";
const int httpPort = 3000;

const int button = 3;
const int led = 4;
const int debounceDelay = 500;
const int activityTimeout = 30000;

volatile bool ringRequested = false;
volatile int lastPressed = 0;

bool ledOn = false;
bool startingUp;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void initializeComm()
{
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(100);
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sendNotify()
{
    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host, httpPort))
    {
        //Serial.println("connection failed");
        return;
    }

    // We now create a URI for the request
    String url = "/messages/event";

    // This will send the request to the server
    client.println("POST /messages/event HTTP/1.1");
    client.println("Host: microraptor.local:300");
    client.println("Content-Type: application/json");
    client.println("Content-Length: " + String(39));
    client.println("Cache-Control: no-cache\n");
    client.print("{\"nodeId\": \"doorbell\",\"event\": \"ring!\"}");

    delay(10);

    // Read all the lines of the reply from server and print them to Serial
    while ( client.available() )
    {
        // Read the response.  We're not going to do anything with it for now.
        char ch = client.read(); //readStringUntil('\r');

        //#ifdef DEBUG
        //    Serial.print(ch);
        //#endif
    }

    client.stop();
    ringRequested = false;
}

/* ------------------- MAIN HANDLERS & LOOP -------------------------------- */

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void flashLed(int repeat)
{
    for (byte i = 0; i < repeat; i++)
    {
        digitalWrite(led, LOW);
        delay(100);
        digitalWrite(led, HIGH);
        delay(100);
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void buttonPressed()
{
    // Handle button debouncing by not handling a button press within 1 second of the previous press.
    if ( !startingUp && !ringRequested && (millis() - lastPressed > debounceDelay) )
    {
        ringRequested = true;
        ledOn = true;
        lastPressed = millis();
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void wakeUp()
{
    sleep_disable();

    // When we wake up, we want to trigger the button press event
    // Remove the wakeUp interrupt handler
    detachInterrupt(button);

    // Restore AVR peripherals used elsewhere in the code
    power_usart0_enable(); // Enable Serial
    power_timer0_enable(); // Enable millis() etc.
    power_spi_enable();    // Enable SPI

    // Set up button to trigger an interrupt.
    attachInterrupt(button, buttonPressed, FALLING);

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
    detachInterrupt(button);
    delay(50);
    attachInterrupt(button, wakeUp, LOW);
    interrupts();
    delay(50);

    digitalWrite(led, LOW);

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    radio.powerDown();

    power_spi_disable();    // Disable SPI
    delay(50);              // Stall for last serial output
    power_timer0_disable(); // Disable millis() etc.
    power_usart0_disable(); // Disable Serial

    // Go to sleep
    sleep_mode();

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

    #ifdef DEVMODE
        Serial.begin(115200);
        delay(10);

        // We start by connecting to a WiFi network

        Serial.println();
        Serial.println();
        Serial.print("Connecting to base station");
    #endif

    initializeComm();

    #ifdef DEVMODE
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    #endif

    pinMode(button, INPUT_PULLUP);
    pinMode(led, OUTPUT);

    flashLed(3);
    ledOn = true;

    // Set up button to trigger an interrupt.
    noInterrupts();
    attachInterrupt(INT1, buttonPressed, FALLING);
    delay(100);
    interrupts();

    ringRequested = false;
    startingUp = false;
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
        #ifdef DEVMODE
            Serial.println( "\nButton pressed" );
            Serial.println("Sending message...");
        #endif

        sendNotify();

        // Reset the ring request flag so button presses are allowed again.
        ringRequested = false;
    }

    int timePassed = millis() - lastPressed;

    if ( timePassed > activityTimeout )
    {
        flashLed(4);
        digitalWrite(led, LOW);
        ledOn = false;

        goToSleep();
    }
    else
    {
        if ( timePassed < 2000 && (0 < (timePassed % 500) < 10) )
        {
            flashLed(1);
        }
    }
}

