#include <Adafruit_NeoPixel.h>

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#ifndef APSSID
#define APSSID "SLOW"
#define APPSK  "kir12345"
#endif

ESP8266WiFiMulti WiFiMulti;

#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 2

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(243, 4, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(240, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() {
  Serial.begin(9600);
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code

  strip.begin();
  strip.setBrightness(255);
  strip.show(); // Initialize all pixels to 'off'
  strip2.begin();
  strip2.setBrightness(255);
  strip2.show(); // Initialize all pixels to 'off'

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(APSSID, APPSK);

  
  strip.setPixelColor(61, strip.Color(0, 50, 0));
  strip.show();
  while((WiFiMulti.run() != WL_CONNECTED))
  {
    if(millis() > 60000)
      break;
    yield();
    strip.setPixelColor((millis()/1000), strip.Color(0, 0, 50));
    strip.show();
    
  }
  if((WiFiMulti.run() == WL_CONNECTED)) {
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  make_update();
  }
}

void loop() {
  // Some example procedures showing how to display to the pixels:
  colorWipe(strip.Color(255, 0, 0), 2); // Red
  colorWipe_inv(strip.Color(255, 0, 0), 2); // Red
  delay(1000);
  colorWipe(strip.Color(0, 255, 0), 2); // Green
  delay(1000);
  colorWipe(strip.Color(0, 0, 255), 2); // Blue
  delay(1000);
  colorWipe(strip.Color(255, 255, 255), 10); // White
  delay(10000);
  colorWipe(strip.Color(0, 0, 0), 10); // Off
  delay(1000);
//colorWipe(strip.Color(0, 0, 0, 255), 50); // White RGBW
  // Send a theater pixel chase in...
  theaterChase(strip.Color(127, 127, 127), 50); // White
  theaterChase(strip2.Color(127, 127, 127), 50); // White
  theaterChase(strip.Color(127, 0, 0), 50); // Red
  theaterChase(strip.Color(0, 0, 127), 50); // Blue

  rainbow(20);
  rainbowCycle(20);
  theaterChaseRainbow(50);
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<(strip.numPixels()+strip2.numPixels()); i++) {
  if(i<strip.numPixels()){
    strip.setPixelColor((strip.numPixels()-1)-i, c);
    strip.show();
  }
  if(i>strip.numPixels())
  {
    strip2.setPixelColor(i-(strip.numPixels()+1), c);
    strip2.show();
  }
    delay(wait);
  }
}
void colorWipe_inv(uint32_t c, uint8_t wait) {
  for(uint16_t i=(strip.numPixels()+strip2.numPixels()); i>0; i--) {
  if(i<strip.numPixels()){
    strip.setPixelColor((strip.numPixels()-1)-i, c);
    strip.show();
  }
  if(i>strip.numPixels())
  {
    strip2.setPixelColor(i-(strip.numPixels()+1), c);
    strip2.show();
  }
    delay(wait);
  }
}
void setPixelColor_full(uint16_t i, uint32_t c){
  if(i<strip.numPixels()){
    strip.setPixelColor((strip.numPixels()-1)-i, c);
  }
  if(i>strip.numPixels())
  {
    strip2.setPixelColor(i-(strip.numPixels()+1), c);
  }
}
void show_full(){
    strip.show();
    strip2.show();
}
unsigned int numPixels_full(){
 return strip.numPixels()+strip2.numPixels();
}
//
//void colorWipe2(uint32_t c, uint8_t wait) {
//  for(uint16_t i=0; i<strip.numPixels(); i++) {
//    strip2.setPixelColor(DIODS2-i, c);
//    strip2.show();
//    delay(wait);
//  }
//}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<numPixels_full(); i++) {
      setPixelColor_full(i, Wheel((i+j) & 255));
    }
    show_full();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< numPixels_full(); i++) {
      setPixelColor_full(i, Wheel(((i * 256 / numPixels_full()) + j) & 255));
    }
    show_full();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < numPixels_full(); i=i+3) {
        setPixelColor_full(i+q, c);    //turn every third pixel on
      }
      show_full();

      delay(wait);

      for (uint16_t i=0; i < numPixels_full(); i=i+3) {
        setPixelColor_full(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < numPixels_full(); i=i+3) {
        setPixelColor_full(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      show_full();

      delay(wait);

      for (uint16_t i=0; i < numPixels_full(); i=i+3) {
        setPixelColor_full(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    return strip2.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    return strip2.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  return strip2.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}


void update_started() {
  Serial.println("CALLBACK:  HTTP update process started");
}

void update_finished() {
  Serial.println("CALLBACK:  HTTP update process finished");
}

void update_progress(int cur, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}

void make_update()
{

  if ((WiFiMulti.run() == WL_CONNECTED)) {

    WiFiClient client;

    // The line below is optional. It can be used to blink the LED on the board during flashing
    // The LED will be on during download of one buffer of data from the network. The LED will
    // be off during writing that buffer to flash
    // On a good connection the LED should flash regularly. On a bad connection the LED will be
    // on much longer than it will be off. Other pins than LED_BUILTIN may be used. The second
    // value is used to put the LED on. If the LED is on with HIGH, that value should be passed
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

    // Add optional callback notifiers
    ESPhttpUpdate.onStart(update_started);
    ESPhttpUpdate.onEnd(update_finished);
    ESPhttpUpdate.onProgress(update_progress);
    ESPhttpUpdate.onError(update_error);

    t_httpUpdate_return ret = ESPhttpUpdate.update(client, "http://dev.a1mc.ru/rom/kir/index.php");
    // Or:
    //t_httpUpdate_return ret = ESPhttpUpdate.update(client, "server", 80, "file.bin");

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
    }
  }
}
