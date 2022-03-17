#include <Adafruit_NeoPixel.h>

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include <aREST.h>

#ifndef APSSID
#define APSSID "SLOW"
#define APPSK  "kir12345"
#endif

ESP8266WiFiMulti WiFiMulti;

aREST rest = aREST();

WiFiServer server(8080);

#ifdef __AVR__
#include <avr/power.h>
#endif

#define STR1 2
#define STR2 4

uint32_t t_color = 0;

bool reboot_flag = false;

unsigned int global_mode = 1;

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(492, STR2, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

int set_mode_r(String command) {

  // Get state from command
  int state = command.toInt();

  return set_mode(state);
}

int reboot(String com) {
  reboot_flag = true;
  ESP.restart();
  return 1;
}

void setup() {
  Serial.begin(9600); //Инициализация uart порта
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  // End of trinket special code

#if defined(BUILDINFO) //Вывод информации о сборку CI/CD
  Serial.println(F(BUILDINFO));
  Serial.println(F(BUILD_SHA));
  Serial.println(F(BUILDTAG));
#endif

  strip.begin(); //Инициализация ленты
  strip.setBrightness(255); //Установка яркости
  fill_full(strip.Color(0, 10, 10), 0, numPixels_full()); //Установка голубово света на всю ленту
  strip.show(); //Вывод цвета на ленту
  WiFi.mode(WIFI_STA); //Установлени режима клиента
  WiFiMulti.addAP(APSSID, APPSK); //Уставка названия сети и пароля для подключения

  while ((WiFiMulti.run() != WL_CONNECTED))//Ожидаем 1 минуту подключение к точке доступа
  {
    if (millis() > 60000)
      break;
    yield();
    strip.setPixelColor((millis() / 1000), strip.Color(0, 0, 50));
    strip.show();

  }
  if ((WiFiMulti.run() == WL_CONNECTED)) { //Если подключение произошло выводим IP и задаём имя хоста
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    WiFi.hostname("ESP_LED");
    make_update();
  }
  //Устанавливаем переменные видные через REST
  rest.variable("global_mode", &global_mode);
  rest.variable("t_color", &t_color);
  //Задаём функциии для REST
  rest.function("set_mode", set_mode_r);
  rest.function("reboot", reboot);

  // Устанавливаем имя устройства & ID для отвев по REST (ID Должем быть не более 6 символов в длинну)
  rest.set_id("1");
  rest.set_name("esp_kir_led");

  server.begin(); //Запускам прослушивания запросов отпользователей
}

//Главный цыкл
void loop() {
  do_mode(); //Обработчик эффектов ленты
  WiFiClient client = server.available();
  if (client) {
    if (client.available()) { //Проверяем если есть клиент передаём его обработчику REST запросов
      rest.handle(client);
      delay(100);
    }
  }
  yield();//Даём модулю исполнить свои подзадачи (по поддержанию wifi соединения)
  if(reboot_flag == true)//Если установлен флаг перезапуска ребутаемся
    ESP.restart();
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < (numPixels_full()); i++) {
    setPixelColor_full(i, c);
    delay(wait);
  }
}
void colorWipe_inv(uint32_t c, uint8_t wait) {
  for (uint16_t i = (numPixels_full()); i > 0; i--) {
    setPixelColor_full(i, c);
    delay(wait);
  }
}
void setPixelColor_full(uint16_t i, uint32_t c) {
  //if(i<strip.numPixels()){
  strip.setPixelColor(i, c);
  //}
  //if(i>strip.numPixels())
  //{
  //  strip2.setPixelColor(i-(strip.numPixels()+1), c);
  //}
}
void show_full() {
  strip.show();
  //strip2.show();
}
unsigned int numPixels_full() {
  return strip.numPixels();
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

  for (j = 255; j >= 0; j--) {
    for (i = 0; i < numPixels_full(); i++) {
      setPixelColor_full(i, Wheel((i + j) & 255));
    }
    show_full();
    delay(wait);
  }
}

void fill_full(uint32_t c, uint16_t first, uint16_t count) {
  uint16_t i, end, numLEDs = numPixels_full();

  if (first >= numLEDs) {
    return; // If first LED is past end of strip, nothing to do
  }

  // Calculate the index ONE AFTER the last pixel to fill
  if (count == 0) {
    // Fill to end of strip
    end = numLEDs;
  } else {
    // Ensure that the loop won't go past the last pixel
    end = first + count;
    if (end > numLEDs)
      end = numLEDs;
  }
  yield();
  for (i = first; i < end; i++) {
    setPixelColor_full(i, c);
    yield();
  }

}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < numPixels_full(); i++) {
      setPixelColor_full(i, Wheel(((i * 256 / numPixels_full()) + j) & 255));
    }
    show_full();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j = 0; j < 10; j++) { //do 10 cycles of chasing
    for (int q = 0; q < 3; q++) {
      for (uint16_t i = 0; i < numPixels_full(); i = i + 3) {
        setPixelColor_full(i + q, c);  //turn every third pixel on
      }
      show_full();

      delay(wait);

      for (uint16_t i = 0; i < numPixels_full(); i = i + 3) {
        setPixelColor_full(i + q, 0);      //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j = 0; j < 256; j++) {   // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++) {
      for (uint16_t i = 0; i < numPixels_full(); i = i + 3) {
        setPixelColor_full(i + q, Wheel( (i + j) % 255)); //turn every third pixel on
      }
      show_full();

      delay(wait);

      for (uint16_t i = 0; i < numPixels_full(); i = i + 3) {
        setPixelColor_full(i + q, 0);      //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void update_started() {//Обновление начато
  Serial.println("CALLBACK:  HTTP update process started");
  fill_full(strip.Color(0, 0, 64), 0, numPixels_full());
  show_full();
}

void make_update() //Проверка и обновление в случае наличия
{
  if ((WiFiMulti.run() == WL_CONNECTED)) {//Проверяем что мы подключены к ТД

    WiFiClient client;

    // The line below is optional. It can be used to blink the LED on the board during flashing
    // The LED will be on during download of one buffer of data from the network. The LED will
    // be off during writing that buffer to flash
    // On a good connection the LED should flash regularly. On a bad connection the LED will be
    // on much longer than it will be off. Other pins than LED_BUILTIN may be used. The second
    // value is used to put the LED on. If the LED is on with HIGH, that value should be passed
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

    //Задаём путь для обращения за обнавлениями
    t_httpUpdate_return ret = ESPhttpUpdate.update(client, "http://dev.a1mc.ru/rom/kir/index.php");

    ESPhttpUpdate.onStart(update_started);

    switch (ret) { //Обработчик результата обнавлений
      case HTTP_UPDATE_FAILED: //Ошибка обновления
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        service_blink(strip.Color(255, 0, 0)); //Мигаем два раза красным сообщая пользователю о проблемме
        break;

      case HTTP_UPDATE_NO_UPDATES: //Обнавления отсутствуют
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        //service_blink(strip.Color(0, 0, 255));
        break;

      case HTTP_UPDATE_OK://Обнавление успешно завершено
        Serial.println("HTTP_UPDATE_OK");
        service_blink(strip.Color(0, 255, 0)); //Мигаем пользователю зелёным сообщая об успешном обновление
        break;
    }
  }
}


void service_blink(uint32_t c) { //Сервисное мигание для отображения статуса
  int led_delim = numPixels_full() / 10;
  yield();
  fill_full(strip.Color(0, 0, 0), 0, numPixels_full());
  show_full();
  delay(1000);
  fill_full(c, led_delim, led_delim + led_delim);
  show_full();
  delay(1000);
  fill_full(strip.Color(0, 0, 0), led_delim, led_delim + led_delim);
  show_full();
  delay(1000);
  fill_full(c, led_delim, led_delim + led_delim);
  show_full();
  delay(1000);
  fill_full(strip.Color(0, 0, 0), led_delim, led_delim + led_delim);
  show_full();
}

void do_mode() //Обработчик эффектов
{
  switch (global_mode) {
    case 0:
      yield();
      break;
    case 1:
      rainbow(20);
      break;
    case 2:
      rainbowCycle(20);
      break;
    case 3:
      theaterChaseRainbow(50);
      break;
    case 4:
      theaterChase(t_color, 50); // White
      break;
    case 5:
      yield();
      break;

  }
}

int set_mode(unsigned int mode_new) //Задать тип эффекта
{
  int ret = 0;
  switch (mode_new) {
    case 0:
      fill_full(strip.Color(0, 0, 0), 0, numPixels_full());
      show_full();
      break;
    case 1:
      rainbow(20);
      break;
    case 2:
      rainbowCycle(20);
      break;
    case 3:
      theaterChaseRainbow(50);
      break;
    case 4:
      theaterChase(t_color, 50); // White
      break;
    case 5:
      colorWipe(t_color, 2); // Blue
      break;
    default:
      ret = 1;
  }
  if (ret == 0)
    global_mode = mode_new;
  return ret;
}
