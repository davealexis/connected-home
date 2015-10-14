/*
 *
 *
 */

#include <ESP8266WiFi.h>

// Comment this line out when creating a build that does not include any debug
// output using Serial.
#define DEVMODE

const char *ssid = "<ssid>";
const char *password = "<network password>";
const char* host = "192.168.1.9";
const int httpPort = 3000;
const int button = 2;
const int debounceDelay = 1000;

volatile bool ringRequested = false;
volatile int lastPressed = 0;

void buttonPressed()
{
    // Handle button debouncing by not handling a button press within 1 second of the previous press.
    if ( !ringRequested && (millis() - lastPressed > debounceDelay) )
    {
        ringRequested = true;
        lastPressed = millis();
    }
}

void setup()
{
    pinMode(button, INPUT_PULLUP);

#ifdef DEVMODE
    Serial.begin(115200);
    delay(10);

    // We start by connecting to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
#endif

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(100);
    }

#ifdef DEVMODE
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
#endif

    // Set up button to trigger an interrupt.
    attachInterrupt(button, buttonPressed, FALLING);
}

void loop()
{
    while ( !ringRequested )
        yield();

#ifdef DEVMODE
    Serial.println( "\nButton pressed" );

    Serial.print("connecting to ");
    Serial.println(host);
#endif

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

#ifdef DEVMODE
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

    delay(50);

    // Read all the lines of the reply from server and print them to Serial
    while ( client.available() )
    {
        char ch = client.read(); //readStringUntil('\r');
        Serial.print(ch);
    }

    client.stop();
    delay(100);

    // Reset the ring request flag so button presses are allowed again.
    ringRequested = false;
}
