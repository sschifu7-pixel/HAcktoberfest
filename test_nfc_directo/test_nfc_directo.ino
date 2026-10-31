/*
 * ==============================================================================
 * Test Unitario Rápido y Continuo: Lector NFC PN532 (I2C) + Avatar Espectral
 * ==============================================================================
 * Conexión Hardware PN532:
 * - Interruptores PN532 para I2C: SET0 = 1 (H), SET1 = 0 (L)
 * - SDA -> GPIO 21
 * - SCL -> GPIO 22
 * - VCC -> 5V (o 3.3V)
 * - GND -> GND
 * 
 * Pantalla TFT 2.8" SPI (ILI9341):
 * - CS=5, DC=4, RST=19 (o 2), SCK=18, MOSI=23
 * ==============================================================================
 */

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_PN532.h>

#define TFT_CS   5
#define TFT_DC   4
#define TFT_RST  19
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

#define PN532_I2C_SDA 21
#define PN532_I2C_SCL 22
Adafruit_PN532 nfc(PN532_I2C_SDA, PN532_I2C_SCL);

#define COLOR_BG          ILI9341_BLACK
#define COLOR_CARD        0x18E3
#define COLOR_PRIMARY     0x03FF
#define COLOR_SUCCESS     0x07E0
#define COLOR_WARNING     0xFDA0
#define COLOR_DANGER      0xF800
#define COLOR_TEXT        ILI9341_WHITE
#define COLOR_YELLOW      0xFFE0

bool pn532OK = false;
uint32_t totalLecturas = 0;

void dibujarCabecera(const char* titulo, uint16_t color) {
  tft.fillRect(0, 0, 320, 32, color);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(12, 8);
  tft.print(titulo);
}

void dibujarFantasmaSimple(int cx, int cy, bool happy) {
  uint16_t bodyColor = happy ? 0xFFF0 : 0xD7FF;
  tft.fillRoundRect(cx - 20, cy + 40, 40, 6, 3, 0x0841); // Sombra
  tft.fillCircle(cx, cy, 24, bodyColor);                  // Cabeza
  tft.fillRect(cx - 24, cy, 48, 30, bodyColor);           // Cuerpo
  tft.fillCircle(cx - 9, cy - 10, 4, ILI9341_WHITE);     // Brillo 3D

  // Ondas cola
  tft.fillCircle(cx - 16, cy + 30, 5, bodyColor);
  tft.fillCircle(cx - 5,  cy + 30, 5, bodyColor);
  tft.fillCircle(cx + 5,  cy + 30, 5, bodyColor);
  tft.fillCircle(cx + 16, cy + 30, 5, bodyColor);
  tft.fillCircle(cx - 10, cy + 34, 3, COLOR_CARD);
  tft.fillCircle(cx,      cy + 34, 3, COLOR_CARD);
  tft.fillCircle(cx + 10, cy + 34, 3, COLOR_CARD);

  int lx = cx - 9, rx = cx + 9, ey = cy + 3;

  if (happy) {
    // Ojos cerrados felices ^ ^
    tft.drawLine(lx - 5, ey + 2, lx, ey - 3, COLOR_WARNING);
    tft.drawLine(lx, ey - 3, lx + 5, ey + 2, COLOR_WARNING);
    tft.drawLine(rx - 5, ey + 2, rx, ey - 3, COLOR_WARNING);
    tft.drawLine(rx, ey - 3, rx + 5, ey + 2, COLOR_WARNING);
    // Sonrojo
    tft.fillCircle(cx - 14, cy + 10, 3, 0xFCB2);
    tft.fillCircle(cx + 14, cy + 10, 3, 0xFCB2);
    // Boca
    tft.fillCircle(cx, cy + 14, 4, COLOR_DANGER);
    tft.fillRect(cx - 5, cy + 9, 10, 5, bodyColor);
    tft.drawPixel(cx - 18, cy - 14, ILI9341_YELLOW);
    tft.drawPixel(cx + 16, cy - 14, ILI9341_YELLOW);
  } else {
    // Ojos azules esperando
    tft.fillCircle(lx, ey, 5, COLOR_PRIMARY);
    tft.fillCircle(lx + 1, ey - 1, 2, 0x0000);
    tft.drawPixel(lx + 2, ey - 2, ILI9341_WHITE);

    tft.fillCircle(rx, ey, 5, COLOR_PRIMARY);
    tft.fillCircle(rx + 1, ey - 1, 2, 0x0000);
    tft.drawPixel(rx + 2, ey - 2, ILI9341_WHITE);

    tft.drawLine(cx - 5, cy + 14, cx + 5, cy + 14, COLOR_PRIMARY);
  }
}

void mostrarPantallaEspera() {
  tft.fillScreen(COLOR_BG);
  dibujarCabecera("TEST DIRECTO NFC PN532", COLOR_PRIMARY);

  // Tarjeta Izquierda con Fantasma
  tft.fillRoundRect(8, 40, 96, 192, 8, COLOR_CARD);
  tft.drawRoundRect(8, 40, 96, 192, 8, COLOR_PRIMARY);
  dibujarFantasmaSimple(56, 105, false);

  tft.fillRoundRect(14, 184, 84, 22, 6, COLOR_BG);
  tft.drawRoundRect(14, 184, 84, 22, 6, COLOR_PRIMARY);
  tft.fillCircle(24, 195, 3, COLOR_PRIMARY);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.setCursor(32, 192);
  tft.print("Escaneando");

  // Tarjeta Derecha Superior: Estado de Hardware
  tft.fillRoundRect(112, 40, 200, 80, 8, COLOR_CARD);
  tft.drawRoundRect(112, 40, 200, 80, 8, pn532OK ? COLOR_SUCCESS : COLOR_DANGER);

  tft.setTextSize(2);
  tft.setCursor(122, 50);
  if (pn532OK) {
    tft.setTextColor(COLOR_SUCCESS);
    tft.print("PN532: LISTO");
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(122, 75);
    tft.print("Bus I2C: 0x24 detectado");
    tft.setCursor(122, 90);
    tft.setTextColor(COLOR_YELLOW);
    tft.printf("Lecturas: %d", totalLecturas);
  } else {
    tft.setTextColor(COLOR_DANGER);
    tft.print("PN532: ERROR");
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(122, 75);
    tft.print("Revise SDA=21, SCL=22");
    tft.setCursor(122, 90);
    tft.print("y switches: SET0=1,SET1=0");
  }

  // Tarjeta Derecha Inferior: Instrucciones
  tft.fillRoundRect(112, 128, 200, 104, 8, COLOR_CARD);
  tft.drawRoundRect(112, 128, 200, 104, 8, COLOR_WARNING);

  tft.setTextSize(2);
  tft.setTextColor(COLOR_WARNING);
  tft.setCursor(122, 140);
  tft.print("Acerque Tag");
  tft.setCursor(122, 160);
  tft.print("NFC / Tarjeta");

  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(122, 190);
  tft.print("Mifare / Tangem / Celular");
  tft.setCursor(122, 205);
  tft.setTextColor(COLOR_SUCCESS);
  tft.print("Lectura continua activa");
}

void mostrarLecturaOK(String uidHex, uint8_t len) {
  tft.fillScreen(COLOR_BG);
  dibujarCabecera("¡TARJETA DETECTADA OK!", COLOR_SUCCESS);

  // Tarjeta Izquierda con Fantasma Feliz
  tft.fillRoundRect(8, 40, 96, 192, 8, COLOR_CARD);
  tft.drawRoundRect(8, 40, 96, 192, 8, COLOR_SUCCESS);
  dibujarFantasmaSimple(56, 105, true);

  tft.fillRoundRect(14, 184, 84, 22, 6, COLOR_BG);
  tft.drawRoundRect(14, 184, 84, 22, 6, COLOR_SUCCESS);
  tft.fillCircle(24, 195, 3, COLOR_SUCCESS);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.setCursor(32, 192);
  tft.print("¡Detectado!");

  // Tarjeta Derecha: Datos de la tarjeta
  tft.fillRoundRect(112, 40, 200, 192, 8, COLOR_CARD);
  tft.drawRoundRect(112, 40, 200, 192, 8, COLOR_SUCCESS);

  tft.setTextSize(2);
  tft.setTextColor(COLOR_SUCCESS);
  tft.setCursor(122, 55);
  tft.print("UID HEX:");

  tft.setTextSize(2);
  tft.setTextColor(COLOR_PRIMARY);
  tft.setCursor(122, 82);
  tft.print(uidHex);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(122, 118);
  tft.printf("Longitud: %d bytes", len);

  tft.setCursor(122, 138);
  tft.setTextColor(COLOR_YELLOW);
  tft.printf("Lectura total #%d", totalLecturas);

  tft.fillRoundRect(122, 168, 180, 42, 6, COLOR_SUCCESS);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(140, 180);
  tft.print("LECTURA OK");
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n--- [TEST DIRECTO NFC PN532 + ESP32] ---");

  // Iniciar TFT
  tft.begin();
  tft.setRotation(1); // 320x240 horizontal
  tft.fillScreen(COLOR_BG);

  // Iniciar PN532 por I2C
  Wire.begin(PN532_I2C_SDA, PN532_I2C_SCL);
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("[ERROR] No se detecto el lector PN532 en bus I2C (0x24).");
    Serial.println("Verifique: SET0=1 (H) y SET1=0 (L) en los interruptores del módulo.");
    pn532OK = false;
  } else {
    pn532OK = true;
    Serial.print("[OK] Lector PN532 detectado! Firmware v");
    Serial.print((versiondata >> 16) & 0xFF, DEC);
    Serial.print('.');
    Serial.println((versiondata >> 8) & 0xFF, DEC);
    nfc.SAMConfig();
  }

  mostrarPantallaEspera();
}

void loop() {
  if (!pn532OK) {
    // Reintentar periódicamente si hubo error al inicio
    delay(2000);
    uint32_t version = nfc.getFirmwareVersion();
    if (version) {
      pn532OK = true;
      nfc.SAMConfig();
      mostrarPantallaEspera();
    }
    return;
  }

  uint8_t success;
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
  uint8_t uidLength = 0;

  // Timeout de 150 ms para no bloquear
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 150);

  if (success) {
    totalLecturas++;
    String uidHex = "";
    for (uint8_t i = 0; i < uidLength; i++) {
      if (uid[i] < 0x10) uidHex += "0";
      uidHex += String(uid[i], HEX);
      if (i < uidLength - 1) uidHex += ":";
    }
    uidHex.toUpperCase();

    Serial.println("\n------------------------------------");
    Serial.printf("[NFC DETECTADO #%d]\n", totalLecturas);
    Serial.print("UID Hex: ");
    Serial.println(uidHex);
    Serial.printf("Longitud: %d bytes\n", uidLength);
    Serial.println("------------------------------------");

    mostrarLecturaOK(uidHex, uidLength);
    delay(2500); // Muestra la tarjeta 2.5 seg
    mostrarPantallaEspera();
  }

  delay(50);
}
