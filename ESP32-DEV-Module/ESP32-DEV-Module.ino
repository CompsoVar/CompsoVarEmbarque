#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#include <MFRC522.h>
#include <SPI.h>


/*

PIN MFRC522 :

SDA(SS) D5 GPIO5
SCK D18 GPIO18
MOSI D23 GPIO23/MOSI
MISO D19 GPIO19/MISO
IRQ NOt USE
GND to GND
RST D21 GPIO21
3.3V to 3.3V


PIN LED:

RED/GREEN VIN/(5V)
GREEN D16 RX2/GPIO16
WHITE GND


*/

// Network credentials
const char* ssid = "ESP32-Network";
const char* password = "Esp32-Password";

const unsigned int localUdpPort = 4210;
WiFiUDP udp;

const uint8_t EXPECTED_UID[] = {0x84, 0xB6, 0x08, 0x02};
const uint8_t EXPECTED_UID_SIZE = sizeof(EXPECTED_UID);

const int pinLed = 2;

// Définition des broches SPI pour ESP32
#define SS_PIN  5  // GPIO5 pour SS (SDA du MFRC522)
#define RST_PIN 36 // GPIO36 pour RESET du MFRC522
MFRC522 RfChip(SS_PIN, RST_PIN);

#define PIN_WS2812B 16  // The ESP32 pin GPIO16 connected to WS2812B
#define NUM_PIXELS 63   // The number of LEDs (pixels) on WS2812B LED strip
Adafruit_NeoPixel ws2812b(NUM_PIXELS, PIN_WS2812B, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  SPI.begin();  // Initialisation SPI obligatoire pour MFRC522
  RfChip.PCD_Init();  // Initialisation du lecteur RFID

  ws2812b.begin();
  ws2812b.clear();
  ws2812b.show();

  pinMode(pinLed, OUTPUT);
  WiFi.softAP(ssid, password, 1, false);
  udp.begin(localUdpPort);

  Serial.println("\nWiFi AP Mode");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
}

void broadcastMessage(const char* message) {
  IPAddress localIp = WiFi.softAPIP();
  IPAddress broadcastIp(localIp[0], localIp[1], localIp[2], 255);
  udp.beginPacket(broadcastIp, localUdpPort);
  udp.write((const uint8_t*)message, strlen(message));
  udp.endPacket();
  Serial.printf("Broadcasted message: %s\n", message);
}

bool compareUID(uint8_t* uid1, uint8_t size1, const uint8_t* uid2, uint8_t size2) {
  if (size1 != size2) return false;
  for (uint8_t i = 0; i < size1; i++) {
    if (uid1[i] != uid2[i]) return false;
  }
  return true;
}

void loop() {
  if (RfChip.PICC_IsNewCardPresent() && RfChip.PICC_ReadCardSerial()) {
    Serial.print("Le tag est le: ");
      for (byte i = 0; i < RfChip.uid.size; i++) {
          Serial.print(RfChip.uid.uidByte[i], HEX);
          Serial.print(" "); // Espace entre chaque octet
      }
            Serial.println();
    if (compareUID(RfChip.uid.uidByte, RfChip.uid.size, EXPECTED_UID, EXPECTED_UID_SIZE)) {
      Serial.println("RFID Tag reconnu !");
      digitalWrite(pinLed, HIGH);
      broadcastMessage("1");
      delay(1000);
      digitalWrite(pinLed, LOW);

      for (int pixel = 0; pixel < 33; pixel++) {
        ws2812b.setPixelColor(pixel, ws2812b.Color(255, 0, 0));
      }
      for (int pixel = 33; pixel < NUM_PIXELS; pixel++) {
        ws2812b.setPixelColor(pixel, ws2812b.Color(255, 255, 255));
      }
      ws2812b.show();
    } else {
      Serial.println("Tag non reconnu !");
      digitalWrite(pinLed, HIGH);
      broadcastMessage("0");
      delay(1000);
      digitalWrite(pinLed, LOW);

      for (int pixel = 0; pixel < 33; pixel++) {
        ws2812b.setPixelColor(pixel, ws2812b.Color(255, 255, 255));
      }
      for (int pixel = 33; pixel < NUM_PIXELS; pixel++) {
        ws2812b.setPixelColor(pixel, ws2812b.Color(255, 0, 0));
      }
      ws2812b.show();
    }
    RfChip.PICC_HaltA();  // Arrêter la communication avec le tag RFID
  }
}
