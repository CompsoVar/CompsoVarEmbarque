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
RED VIN/(5V)
Yellow D16 RX2/GPIO16
Black GND
*/

// Network credentials
const char* ssid = "Compsognathus-Network";
const char* password = "Compsognathus-Password";

const unsigned int localUdpPort = 4210;
WiFiUDP udp;

const uint8_t EXPECTED_UID[] = {0x7F, 0xE8, 0x02, 0x51};
const uint8_t EXPECTED_UID_SIZE = sizeof(EXPECTED_UID);

const int pinLed = 2;

// Définition des broches SPI pour ESP32
#define SS_PIN  5 
#define RST_PIN 21 
#define MOSI  23  
#define MISO 19 
#define SCK  18  

MFRC522 RfChip(SS_PIN, RST_PIN);

#define PIN_WS2812B 16  // The ESP32 pin GPIO16 connected to WS2812B
#define NUM_PIXELS 8   // The number of LEDs (pixels) on WS2812B LED strip
Adafruit_NeoPixel ws2812b(NUM_PIXELS, PIN_WS2812B, NEO_GRB + NEO_KHZ800);

// Couleurs de base
#define BASE_COLOR_RED    0xFF0000  // Rouge pour aucun tag
#define BASE_COLOR_GREEN  0x00FF00  // Vert pour tag non reconnu
#define BASE_COLOR_BLUE   0x0000FF  // Bleu pour tag reconnu

// Paramètres d'animation des couleurs statiques
// AJUSTEZ CETTE VALEUR POUR CONTRÔLER L'INTENSITÉ DE L'ANIMATION (0.0 = désactivée, 1.0 = maximum)
float COLOR_VARIATION_INTENSITY = 0.5;  // 15% de variation par défaut

// Variables pour suivre l'état du tag
uint8_t lastTagUID[10] = {0};
uint8_t lastTagSize = 0;
bool tagPresent = false;
bool messageSent = false;
unsigned long lastRFIDCheck = 0;
const unsigned long RFID_CHECK_INTERVAL = 50; // ms entre les vérifications RFID

// Variables pour la transition de couleur
uint32_t currentBaseColor = BASE_COLOR_RED;
uint32_t targetBaseColor = BASE_COLOR_RED;
unsigned long transitionStartTime = 0;
const unsigned long TRANSITION_DURATION = 800; // Durée de transition en ms (augmentée pour plus de fluidité)
bool isTransitioning = false;

// Déclarer des Tasks pour ESP32
TaskHandle_t LEDUpdateTask;

void setup() {
  Serial.begin(115200);
  SPI.begin(SCK, MISO, MOSI, SS_PIN);
  RfChip.PCD_Init();

  ws2812b.begin();
  ws2812b.clear();
  for (int i = 0; i < NUM_PIXELS; i++) {
    ws2812b.setPixelColor(i, ws2812b.Color(255, 0, 0));
  }
  ws2812b.show();
  
  pinMode(pinLed, OUTPUT);
  WiFi.softAP(ssid, password, 1, false);
  udp.begin(localUdpPort);

  Serial.println("\nWiFi AP Mode");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
  
  // Créer une tâche séparée pour la mise à jour des LEDs
  xTaskCreatePinnedToCore(
    updateLEDsTask,   // Fonction de tâche
    "LEDUpdate",      // Nom pour débogage
    4096,             // Taille de la pile
    NULL,             // Paramètres de la tâche
    1,                // Priorité
    &LEDUpdateTask,   // Handle de la tâche
    0);               // Core où exécuter (0 pour ESP32)
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

bool isSameTag(uint8_t* uid1, uint8_t size1, uint8_t* uid2, uint8_t size2) {
  return compareUID(uid1, size1, uid2, size2);
}

// Fonction d'easing quadratique pour des transitions douces
float easeInOutQuad(float t) {
  return t < 0.5 ? 2 * t * t : 1 - pow(-2 * t + 2, 2) / 2;
}

// Extraire les composantes RGB d'une couleur 32 bits
void getRGB(uint32_t color, uint8_t &r, uint8_t &g, uint8_t &b) {
  r = (color >> 16) & 0xFF;
  g = (color >> 8) & 0xFF;
  b = color & 0xFF;
}

// Fonction pour démarrer une transition vers une nouvelle couleur
void startColorTransition(uint32_t newTargetColor) {
  if (targetBaseColor != newTargetColor) {
    currentBaseColor = calculateBaseTransitionColor(); // Capture la couleur actuelle réelle
    targetBaseColor = newTargetColor;
    transitionStartTime = millis();
    isTransitioning = true;
    Serial.println("Démarrage d'une nouvelle transition de couleur");
  }
}

// Fonction pour calculer la couleur de base intermédiaire lors d'une transition
uint32_t calculateBaseTransitionColor() {
  if (!isTransitioning) {
    return targetBaseColor;
  }
  
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - transitionStartTime;
  
  // Si la transition est terminée
  if (elapsedTime >= TRANSITION_DURATION) {
    isTransitioning = false;
    return targetBaseColor;
  }
  
  // Calculer la progression de la transition (0.0 à 1.0) avec easing
  float rawProgress = (float)elapsedTime / TRANSITION_DURATION;
  float progress = easeInOutQuad(rawProgress); // Applique l'easing pour une transition plus douce
  
  // Extraire les composantes RGB des couleurs de départ et d'arrivée
  uint8_t startR, startG, startB;
  uint8_t endR, endG, endB;
  
  getRGB(currentBaseColor, startR, startG, startB);
  getRGB(targetBaseColor, endR, endG, endB);
  
  // Calculer les composantes intermédiaires
  uint8_t r = startR + (endR - startR) * progress;
  uint8_t g = startG + (endG - startG) * progress;
  uint8_t b = startB + (endB - startB) * progress;
  
  return ws2812b.Color(r, g, b);
}

// Applique une variation animée à une couleur de base
// Applique une variation animée à une couleur de base
uint32_t applyColorAnimation(uint32_t baseColor) {
  // Calculer des valeurs sinusoïdales qui oscillent entre -1 et 1 basées sur le temps
  float phase1 = (float)(millis() % 5000) / 5000.0f * 2 * PI;
  float phase2 = (float)(millis() % 7000) / 7000.0f * 2 * PI; // Cycle différent pour plus de variété
  float sinValue = sin(phase1);
  float cosValue = cos(phase2);
  
  // Extraire les composantes RGB de la couleur de base
  uint8_t baseR, baseG, baseB;
  getRGB(baseColor, baseR, baseG, baseB);
  
  // Déterminer la composante dominante et sa valeur
  uint8_t dominantValue = max(max(baseR, baseG), baseB);
  
  // Calculer des variations pour chaque composante
  int r, g, b;
  
  // Pour le rouge
  if (baseR > 0) {
    // Si le rouge existe déjà, faire varier sa valeur
    r = baseR * (1.0f + sinValue * COLOR_VARIATION_INTENSITY * 0.7f);
  } else {
    // Si le rouge est à 0, ajouter une petite quantité pour l'animation
    r = dominantValue * COLOR_VARIATION_INTENSITY * 0.2f * (0.5f + 0.5f * sinValue);
  }
  
  // Pour le vert
  if (baseG > 0) {
    // Si le vert existe déjà, faire varier sa valeur
    g = baseG * (1.0f + cosValue * COLOR_VARIATION_INTENSITY * 0.7f);
  } else {
    // Si le vert est à 0, ajouter une petite quantité pour l'animation
    g = dominantValue * COLOR_VARIATION_INTENSITY * 0.2f * (0.5f + 0.5f * cosValue);
  }
  
  // Pour le bleu
  if (baseB > 0) {
    // Si le bleu existe déjà, faire varier sa valeur
    b = baseB * (1.0f + (sinValue * cosValue) * COLOR_VARIATION_INTENSITY * 0.7f);
  } else {
    // Si le bleu est à 0, ajouter une petite quantité pour l'animation
    b = dominantValue * COLOR_VARIATION_INTENSITY * 0.2f * (0.5f + 0.5f * (sinValue * cosValue));
  }
  
  // S'assurer que les valeurs restent dans la plage 0-255
  r = constrain(r, 0, 255);
  g = constrain(g, 0, 255);
  b = constrain(b, 0, 255);
  
  return ws2812b.Color(r, g, b);
}

// Tâche dédiée pour la mise à jour des LEDs
void updateLEDsTask(void * parameter) {
  while (true) {
    // Obtenir la couleur de base (statique ou en transition)
    uint32_t baseColor = calculateBaseTransitionColor();
    
    // Appliquer l'animation à cette couleur de base
    uint32_t animatedColor = applyColorAnimation(baseColor);
    
    for (int i = 0; i < NUM_PIXELS; i++) {
      ws2812b.setPixelColor(i, animatedColor);
    }
    ws2812b.show();
    
    // Délai très court pour des mises à jour ultra-fluides
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void checkRFID() {
  // Vérifier si un tag est présent
  if (RfChip.PICC_IsNewCardPresent() && RfChip.PICC_ReadCardSerial()) {
    // Un tag est détecté
    
    // Si c'est un nouveau tag ou si un tag différent est présenté
    if (!tagPresent || !isSameTag(RfChip.uid.uidByte, RfChip.uid.size, lastTagUID, lastTagSize)) {
      // Mémoriser ce tag comme le dernier détecté
      memcpy(lastTagUID, RfChip.uid.uidByte, RfChip.uid.size);
      lastTagSize = RfChip.uid.size;
      tagPresent = true;
      messageSent = false; // Réinitialiser l'état d'envoi pour un nouveau tag
      
      Serial.print("Nouveau tag détecté: ");
      for (byte i = 0; i < RfChip.uid.size; i++) {
          Serial.print(RfChip.uid.uidByte[i], HEX);
          Serial.print(" ");
      }
      Serial.println();
    }
    
    // Configuration de la couleur et envoi du message si nécessaire
    if (compareUID(RfChip.uid.uidByte, RfChip.uid.size, EXPECTED_UID, EXPECTED_UID_SIZE)) {
      // Tag reconnu - BLEU
      startColorTransition(BASE_COLOR_BLUE);
      
      if (!messageSent) {
        Serial.println("RFID Tag reconnu !");
        digitalWrite(pinLed, HIGH);
        broadcastMessage("1");
        delay(200); // Bref délai pour le clignotement de la LED
        digitalWrite(pinLed, LOW);
        messageSent = true;
      }
    } else {
      // Tag non reconnu - VERT
      startColorTransition(BASE_COLOR_GREEN);
      
      if (!messageSent) {
        Serial.println("Tag non reconnu !");
        digitalWrite(pinLed, HIGH);
        broadcastMessage("0");
        delay(200); // Bref délai pour le clignotement de la LED
        digitalWrite(pinLed, LOW);
        messageSent = true;
      }
    }
    
    RfChip.PICC_HaltA();  // Arrêter la communication avec le tag RFID
  } else {
    // Vérifier si le tag a été retiré
    if (tagPresent) {
      if (!RfChip.PICC_IsNewCardPresent()) {
        // Le tag a été retiré
        startColorTransition(BASE_COLOR_RED);
        tagPresent = false;
        messageSent = false;
        Serial.println("Tag retiré");
      }
    }
  }
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Vérifier le lecteur RFID à des intervalles réguliers
  if (currentMillis - lastRFIDCheck >= RFID_CHECK_INTERVAL) {
    lastRFIDCheck = currentMillis;
    checkRFID();
    RfChip.PCD_Init(); // Réinitialiser le lecteur RFID
  }
  
  // Délai minimal pour céder le CPU à d'autres tâches
  delay(5);
}
