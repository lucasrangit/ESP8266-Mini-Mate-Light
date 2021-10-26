#include <Arduino.h>

// For LED
#include <FastLED.h>

// For WiFi
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESPAsyncUDP.h>
#include <WiFiManager.h>

#define DATA_PIN     0
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
#define CRAP_PACKET_SIZE (NUM_LEDS*3+4)
char packet[CRAP_PACKET_SIZE] = {0}; // this is limited by lwIP
char crap[CRAP_PACKET_SIZE] = {0};
int crap_idx = 0;
bool packet_received = false;

static const int matrix_arrangement[] = { 0,1,2,3,4,5,6,7,8,9 };

static void club_matrix_render(char crap[])
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        int led_real_x = i % WIDTH; // 0 to 39
        int led_real_y = i / WIDTH; // 0 to 1

        int matrix_number_x = led_real_x / 8; // should be 0 to 4
        int matrix_number_y = led_real_y / 8; // should be 0 or 1

        int matrix_number = matrix_number_y * 5 + matrix_number_x; // 0 to 9

        int matrix_x = led_real_x % 8; // 0 to 7
        int matrix_y = led_real_y % 8; // 0 to 7

        leds[matrix_arrangement[matrix_number] * 64 + matrix_y * 8 + matrix_x].r = crap[i*3];
        leds[matrix_arrangement[matrix_number] * 64 + matrix_y * 8 + matrix_x].g = crap[i*3+1];
        leds[matrix_arrangement[matrix_number] * 64 + matrix_y * 8 + matrix_x].b = crap[i*3+2];
    }
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(LED_BUILTIN, OUTPUT);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  Serial.println("Power-up safety delay...");
  delay(3000);

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(true);
  wifiManager.setConfigPortalTimeout(3*60);
  wifiManager.autoConnect("MiniMateLight");

  if (udp.listen(UDP_PORT)) {
    udp.onPacket([](AsyncUDPPacket packet) {
      // Serial.printf("Received %d bytes from %s, port %d\n", packet.length(), packet.remoteIP().toString().c_str(), packet.remotePort());
      // Serial.print("Data: ");
      // // Serial.write(packet.data(), packet.length());
      // for (int i = 0; i < packet.length(); ++i)
      //   Serial.print(*(packet.data() + i));
      // Serial.println();

      // quick hack to reassemble packets
      memcpy(crap + crap_idx, packet.data(), packet.length());
      crap_idx += packet.length();
      if (crap_idx >= CRAP_PACKET_SIZE) {
        club_matrix_render(crap);
        FastLED.show();
        crap_idx = 0;
        memset(crap, 0, sizeof(crap));
      }

      packet_received = true;
    });
  }
}

static void fill_rainbow_matrix( struct CRGB * pFirstLED, int numToFill,
                  uint8_t initialhue,
                  uint8_t deltahue )
{
    CHSV hsv;
    hsv.hue = initialhue;
    hsv.val = 255;
    hsv.sat = 240;
    for (int i = 0; i < numToFill; ++i) {
      int led_real_x = i % WIDTH; // 0 to 39
      int led_real_y = i / WIDTH; // 0 to 1

      int matrix_number_x = led_real_x / 8; // should be 0 to 4
      int matrix_number_y = led_real_y / 8; // should be 0 or 1

      int matrix_number = matrix_number_y * 5 + matrix_number_x; // 0 to 9

      int matrix_x = led_real_x % 8; // 0 to 7
      int matrix_y = led_real_y % 8; // 0 to 7

      pFirstLED[matrix_arrangement[matrix_number] * 64 + matrix_y * 8 + matrix_x] = hsv;
      hsv.hue += deltahue;
    }
}

static void show_test_pattern() {
  static int initialHue = CRGB::Black;
  static int hueDensity = 8*5;
  static int deltaHue = 1;

  if (!packet_received) {
    fill_rainbow_matrix(leds, NUM_LEDS, initialHue += hueDensity, deltaHue);
    FastLED.show();
  }
}

void loop() {
  static int led_state = LOW;
  digitalWrite(LED_BUILTIN, led_state);
  delay(500);
  led_state = led_state == LOW ? HIGH : LOW;

  show_test_pattern();
}