#include <Arduino.h>

// For LED
#include <FastLED.h>

// For WiFi
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESPAsyncUDP.h>
#include <WiFiManager.h>

#define LED_PIN     3
#define WIDTH       40
#define HEIGHT      16
#define NUM_LEDS    (WIDTH*HEIGHT) // (X*Y)
#define BRIGHTNESS  10

CRGB leds[NUM_LEDS] = {0};

#define UDP_PORT 1337

AsyncUDP udp;

// CRAP (Custom advanced video stReAming Protocol) packet contains three bytes of RGB data per pixel in 16 rows of 40 columns.
// [R0,0 G0,0 B0,0 R0,1 G0,1 B0,1 ... R0,39 G0,39 B0,39 R1,0 G1,0 B1,0 ... R15,39 G15,39 B15,39])
// A CRC-32 can optionally be added at the end of the packet.
char packet[NUM_LEDS*3+4] = {0};

void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("Power-up safety delay...");
  delay(3000);

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(true);
  wifiManager.autoConnect("MiniMateLight");

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  if (udp.listen(UDP_PORT)) {
      udp.onPacket([](AsyncUDPPacket packet) {
          Serial.printf("Received %d bytes from %s, port %d\n", packet.length(), packet.remoteIP().toString().c_str(), packet.remotePort());
          Serial.print("Data: ");
          // Serial.write(packet.data(), packet.length());
          for (int i = 0; i < packet.length(); ++i)
            Serial.print(*(packet.data() + i));
          Serial.println();
      });
  }
}

static int matrix_arrangement[] = { 0,1,2,3,4,5,6,7,8,9 };
static void club_matrix_render(uint8_t matrix[])
{
    for (int i=0; i< NUM_LEDS; i++)
    {
        int led_real_x = i % WIDTH; // 0 to 39
        int led_real_y = i / WIDTH; // 0 to 1

        int matrix_number_x = led_real_x / 8; // should be 0 to 4
        int matrix_number_y = led_real_y / 8; // should be 0 or 1

        int matrix_number = matrix_number_y * 5 + matrix_number_x; // 0 to 9

        int matrix_x = led_real_x % 8; // 0 to 7
        int matrix_y = led_real_y % 8; // 0 to 7

        leds[matrix_arrangement[matrix_number] * 64 + matrix_y * 8 + matrix_x] = matrix[i];
    }
}

void loop() {
  static int led_state = LOW;
  digitalWrite(LED_BUILTIN, led_state);
  delay(1000);
  led_state = led_state == LOW ? HIGH : LOW;
}