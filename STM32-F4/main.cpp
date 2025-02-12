// Test of cheap 13.56 MHz RFID-RC522 module for STM32 (Nucleo F401RE)
//
// Connections:
// RFID pins       -> Nucleo header CN5 (Arduino-compatible header):
// ---------------------------------------------------------------
// RFID IRQ=pin5   ->   Not used. Leave open
// RFID MISO=pin4  ->   Nucleo SPI_MISO=PA_6=D12
// RFID MOSI=pin3  ->   Nucleo SPI_MOSI=PA_7=D11
// RFID SCK=pin2   ->   Nucleo SPI_SCK=PA_5=D13
// RFID SDA=pin1   ->   Nucleo SPI_CS=PB_6=D10
// RFID RST=pin7   ->   Nucleo =PA_9=D8
// 3.3V and GND to respective pins.

#include "mbed.h"
#include "MFRC522.h"
#include <cstdio>

#define MF_RESET D8

DigitalOut LedGreen(LED1);

BufferedSerial pc(USBTX, USBRX, 115200);

MFRC522 RfChip(SPI_MOSI, SPI_MISO, SPI_SCK, SPI_CS, MF_RESET);

// UID à comparer (en hexadécimal)
const uint8_t EXPECTED_UID[] = {0x04, 0x15, 0x26, 0x32, 0x72, 0x40, 0x81};
const uint8_t EXPECTED_UID_SIZE = 7;

bool compareUID(uint8_t* uid1, uint8_t size1, const uint8_t* uid2, uint8_t size2) {
    if (size1 != size2) return false;
    
    for (uint8_t i = 0; i < size1; i++) {
        if (uid1[i] != uid2[i]) return false;
    }
    return true;
}

int main() {
    pc.write("starting...\n", 11);
    RfChip.PCD_Init();

    while (true) {
        LedGreen = 0;

        if (!RfChip.PICC_IsNewCardPresent()) {
            ThisThread::sleep_for(500ms);
            continue;
        }

        if (!RfChip.PICC_ReadCardSerial()) {
            ThisThread::sleep_for(500ms);
            continue;
        }

        // Comparer l'UID
        if (compareUID(RfChip.uid.uidByte, RfChip.uid.size, EXPECTED_UID, EXPECTED_UID_SIZE)) {
            pc.write("1\n", 2);
        } else {
            pc.write("0\n", 2);
        }

        ThisThread::sleep_for(1s);
    }
}