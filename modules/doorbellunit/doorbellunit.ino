/*
 *
 *
 */

/*
 *  BUILD CONTROL
 *  ---------------------------------------------------------------------------
 *  Define the DEVMODE flag to include serial debug output.
 *  Define the COMM_WIFI flag to use ESP8266 WiFi module for communication or
 *  COMM_NRF24 to use NRF24L01+ modules.
 */

#include <avr/sleep.h>
#include <avr/power.h>

//#define DEVMODE
#define COMM_NRF24

#ifdef COMM_WIFI
    #include <ESP8266WiFi.h>
    // #include "comm_esp8266.cpp"
#endif

#ifdef COMM_NRF24

    #include <SPI.h>
    #include <RF24_config.h>
    #include <nRF24L01.h>
    #include <RF24.h>
    // #include "comm_nrf24l01.cpp"

    // Pro Trinket
    #define CE_PIN   9
    #define CSN_PIN  10

    // Trinket
//    #define CE_PIN   3
//    #define CSN_PIN  4

    // Uno
    //#define CE_PIN  9
    //#define CSN_PIN 10

    const uint64_t pipe = 0xE8E8F0F0E1LL; // Define the transmit pipe

    int message[2];

    RF24 radio(CE_PIN, CSN_PIN);
#endif


#ifdef COMM_WIFI
    const char *ssid = "<ssid>";
    const char *password = "<network password>";
    const char* host = "192.168.1.9";
    const int httpPort = 3000;
#endif

const int button = 3;
const int led = 4;
const int debounceDelay = 500;

volatile bool ringRequested = false;
volatile int lastPressed = 0;

bool ledOn = false;


/* ------------------- ESP8266 ----------------------------------------- */

#ifdef COMM_WIFI

    void initializeComm()
    {
        WiFi.begin(ssid, password);

        while (WiFi.status() != WL_CONNECTED)
        {
            delay(100);
        }
    }

    void sendNotify()
    {
        // Use WiFiClient class to create TCP connections
        WiFiClient client;

        if (!client.connect(host, httpPort))
        {
            #ifdef DEVMODE
                Serial.println("connection failed");
            #endif
            return;
        }

        // We now create a URI for the request
        String url = "/messages/event";

        #ifdef DEBUG
          Serial.print("Requesting URL: ");
          Serial.println(url);
        #endif

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

            #ifdef DEBUG
                Serial.print(ch);
            #endif
        }

        client.stop();
        ringRequested = false;
    }

#endif

/* ------------------- NRF24L01+ ----------------------------------------- */

#ifdef COMM_NRF24

    void initializeComm()
    {
        message[0] = 517;
        message[1] = 1;

        radio.begin();
        radio.openWritingPipe( pipe );

        delay(50);
    }

    void sendNotify()
    {
        radio.write( message, sizeof( message ) );

        flashLed(4);
    }

#endif

/* ------------------- MAIN HANDLERS & LOOP -------------------------------- */

void flashLed(byte repeat)
{
    for (byte i = 0; i < repeat; i++)
    {
        digitalWrite(led, LOW);
        delay(100);
        digitalWrite(led, HIGH);
        delay(100);
    }
}

void buttonPressed()
{
    // Handle button debouncing by not handling a button press within 1 second of the previous press.
    if ( !ringRequested && (millis() - lastPressed > debounceDelay) )
    {
        ringRequested = true;
        ledOn = true;
        lastPressed = millis();
    }
}

void wakeUp()
{
    sleep_disable();

    // When we wake up, we want to trigger the button press event
    // Remove the wakeUp interrupt handler
    detachInterrupt(INT1);

    #ifdef DEVMODE
      Serial.println("wake!");
    #endif

    // Restore AVR peripherals used elsewhere in the code
    power_usart0_enable(); // Enable Serial
    power_timer0_enable(); // Enable millis() etc.
    power_spi_enable();    // Enable SPI

    // Turn on the NRF24L01+ radio.
    radio.powerUp();

    // Set up button to trigger an interrupt.
    attachInterrupt(INT1, buttonPressed, FALLING);

    // Simulate button press to set the correct flags
    buttonPressed();
}

void enterSleep(void)
{
    noInterrupts();
    detachInterrupt(INT1);
    delay(50);
    attachInterrupt(INT1, wakeUp, LOW);
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


void setup()
{
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
    #ifdef COMM_WIFI
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    #endif
    #endif

    ringRequested = false;

    pinMode(button, INPUT_PULLUP);
    pinMode(led, OUTPUT);

    flashLed(3);
    ledOn = true;

    // Set up button to trigger an interrupt.
    noInterrupts();
    attachInterrupt(INT1, buttonPressed, FALLING);
    delay(50);
    interrupts();
}

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

    if ( ledOn )
    {
        int timePassed = millis() - lastPressed;

        if ( timePassed > 20000 )
        {
            flashLed(4);
            digitalWrite(led, LOW);
            ledOn = false;

            enterSleep();
        }
        else
        {
            if ( timePassed < 2000 && (0 < (timePassed % 500) < 10) )
            {
                flashLed(1);
            }
        }
    }
}

