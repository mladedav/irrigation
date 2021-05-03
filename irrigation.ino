#include <ESP8266WiFi.h>
#include <EEPROM.h>

#include <WiFiUdp.h>
#include <coap-simple.h>

#include <DHT.h>

#define ESP_SSID "Zvirata"
#define ESP_PASS // Fill me

#define IOT_GATEWAY "192.168.0.203"
const int PORT = 5683;
const char PATH[] = "measurement";

#define SECOND 1e6

// Fun fact this pin is also needed for deep sleep wakeup
// So do not use the LED
// const int LED_PIN = 16;
const int RELAY_PIN = 15;
const int DHT_PIN = 0;
const int MOISTURE_PIN = A0;

const int BOOT_NUM_ADDR = 0;

DHT dht(DHT_PIN, DHT11);

WiFiUDP Udp;
Coap coap(Udp);
char buffer[512];

const char VERSION[] = "1.0.0";
uint32_t bootNumber;

void setup()
{
    Serial.begin(74880);

    Serial.print("Connecting...");
    WiFi.begin(ESP_SSID, ESP_PASS);
 
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());

    // This port is used for sending. Not sure if this is needed.
    Udp.begin(1337);

    // rand is used by coap so we need to seed it
    srand(RANDOM_REG32);

    EEPROM.begin(4);
    EEPROM.get(BOOT_NUM_ADDR, bootNumber);
    bootNumber++;
    EEPROM.put(BOOT_NUM_ADDR, bootNumber);
    EEPROM.end();

    Serial.printf("Started with version %s and boot number %u\n", VERSION, bootNumber);
}

// the loop function runs over and over again forever
void loop()
{
    Serial.println("Running");

    // DHT11 - Temp & Humidity
    dht.begin();
    int readData = dht.read();
	float t = dht.readTemperature();
	float h = dht.readHumidity();
    Serial.printf("Temp: %.1f\nHum: %.1f\n", t, h);

    // Moisture
    int m = analogRead(MOISTURE_PIN);
    Serial.printf("Moisture: %d\n", m);

    // Serialize JSON
    // Simple CoAP supports buffers of max length 128. That's including headers and options
    sprintf(buffer, "{\"ver\":\"%s\",\"bootId\":%u,\"temperature\":%.1f,\"humidity\":%.1f,\"moisture\":%d}", VERSION, bootNumber, t, h, m);

    Serial.printf("Buffer length: %d\nBuffer content: %s\n", strlen(buffer), buffer);

    IPAddress ip;
    ip.fromString(IOT_GATEWAY);

    Serial.printf("Sending to IP %s\n", ip.toString().c_str());
    Serial.printf("Sending coap message\n");
    coap.send(ip, PORT, PATH, COAP_NONCON, COAP_POST, (const uint8_t *)"", 0, (const uint8_t *)buffer, strlen(buffer));
    Serial.printf("Coap message sent\n");

    // Milliseconds - wait one second
    delay(1e3);
    delay(60 * 1e3);

    Serial.println("Finished delaying. All network operations should be done. Going to deep sleep.");

    ESP.deepSleep(5 * 60 * SECOND);
}
