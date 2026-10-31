/*
 * ESP32 Web3 Vending Machine - Lector PN532 I2C + TFT 2.8" (ILI9341) + UART Pico
 * Con Avatar Espectral "IAFantasma" Animado por Emociones (Adafruit_GFX)
 * 
 * Hardware Config:
 * - Lector PN532 en modo I2C (SET0=H / 1, SET1=L / 0):
 *     SDA (MO/SDA/TX) -> GPIO 21
 *     SCL (NSS/SCL/RX)-> GPIO 22
 *     5V              -> 5V / VIN
 *     GND             -> GND
 * - Pantalla TFT 2.8" SPI (ILI9341):
 *     CS=5, DC=4, RST=2, SCK=18 (CLK), MOSI=23 (SDA), MISO=19
 * - UART2 a Raspberry Pi Pico:
 *     RX2=16 (conecta a GP0/TX de Pico)
 *     TX2=17 (conecta a GP1/RX de Pico)
 */

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_PN532.h>
#include <HardwareSerial.h>

// 1. Pines Pantalla TFT 2.8" ILI9341 (SPI VSPI)
#define TFT_CS   5
#define TFT_DC   4
#define TFT_RST  2
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// 2. Pines NFC PN532 por I2C (GPIO 21 SDA, GPIO 22 SCL)
#define PN532_I2C_SDA 21
#define PN532_I2C_SCL 22
Adafruit_PN532 nfc(PN532_I2C_SDA, PN532_I2C_SCL);

// 3. UART2 para Comunicación con la Raspberry Pi Pico
#define RX2_PIN 16
#define TX2_PIN 17
HardwareSerial PicoSerial(2);

// Paleta de Colores RGB565 para la Interfaz y el Fantasma
#define COLOR_BG          ILI9341_BLACK
#define COLOR_CARD        0x18E3 // Gris Oscuro Carcasa
#define COLOR_CARD_BORDER 0x3186 // Borde sutil
#define COLOR_PRIMARY     0x03FF // Cían / Azul Neón
#define COLOR_SUCCESS     0x07E0 // Verde Neón
#define COLOR_WARNING     0xFDA0 // Naranja / Ámbar
#define COLOR_DANGER      0xF800 // Rojo Espectral
#define COLOR_TEXT        ILI9341_WHITE
#define COLOR_MUTED       0x9CD3 // Gris Claro

// Colores específicos para el Fantasma
#define GHOST_WHITE       0xFFFF // Blanco brillante
#define GHOST_ICE_BLUE    0xD7FF // Azul hielo (Thinking)
#define GHOST_GOLD_WHITE  0xFFF0 // Blanco dorado (Happy)
#define GHOST_SHOCKED     0xFFB0 // Tinte sorpresa
#define GHOST_CHEEK_PINK  0xFCB2 // Sonrojo mejillas
#define GHOST_SHADOW      0x0841 // Sombra suelo

// Estados emocionales del Fantasma
enum GhostEmotion {
  GHOST_IDLE,       // Reposo amigable, ojos rojos clásicos y parpadeo
  GHOST_THINKING,   // Esperando tarjeta/proceso, ojos azul cielo y signo ?
  GHOST_HAPPY,      // Pago confirmado/despachando, ojos en arco ^ ^ y estrellas
  GHOST_SURPRISED   // Error/sin caída, ojos enormes y rayos de susto
};

int canalSeleccionado = 0; // 0 = Reposo, 1 = Canal A1, 2 = Canal A2
bool esperandoPago = false;
unsigned long ultimoEscaneo = 0;
GhostEmotion emocionActual = GHOST_IDLE;

// Dibuja el encabezado superior estilizado
void dibujarEncabezado(const char* titulo, uint16_t colorHeader) {
  tft.fillRect(0, 0, 320, 36, colorHeader);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(12, 10);
  tft.print(titulo);
}

// =========================================================================
// RENDERIZADO DEL FANTASMA CON EXPRESIONES SEGÚN EMOCIÓN (Adafruit_GFX)
// =========================================================================
void dibujarFantasma(int cx, int cy, GhostEmotion emocion) {
  uint16_t bodyColor = GHOST_WHITE;

  switch (emocion) {
    case GHOST_THINKING:  bodyColor = GHOST_ICE_BLUE;   break;
    case GHOST_HAPPY:     bodyColor = GHOST_GOLD_WHITE;  break;
    case GHOST_SURPRISED: bodyColor = GHOST_SHOCKED;     break;
    default:              bodyColor = GHOST_WHITE;       break;
  }

  // 1. Sombra bajo el fantasma
  tft.fillRoundRect(cx - 22, cy + 45, 44, 7, 3, GHOST_SHADOW);

  // 2. Anillo orbital si está pensando
  if (emocion == GHOST_THINKING) {
    tft.drawCircle(cx, cy + 10, 34, 0x0276);
    tft.drawCircle(cx, cy + 10, 35, 0x041F);
  }

  // 3. Cabeza abovedada (Cúpula)
  int r = 26;
  tft.fillCircle(cx, cy, r, bodyColor);

  // 4. Torso principal
  tft.fillRect(cx - r, cy, r * 2, 34, bodyColor);

  // 5. Destello de brillo superior izquierdo (estilo 3D suave)
  tft.fillCircle(cx - 10, cy - 11, 4, ILI9341_WHITE);

  // 6. Pliegues ondulados inferiores (cola del fantasma)
  int tailY = cy + 34;
  tft.fillCircle(cx - 19, tailY, 6, bodyColor);
  tft.fillCircle(cx - 6,  tailY, 6, bodyColor);
  tft.fillCircle(cx + 6,  tailY, 6, bodyColor);
  tft.fillCircle(cx + 19, tailY, 6, bodyColor);

  // Recortes oscuros para crear el efecto ondulado de la falda
  tft.fillCircle(cx - 13, tailY + 5, 4, COLOR_CARD);
  tft.fillCircle(cx,      tailY + 5, 4, COLOR_CARD);
  tft.fillCircle(cx + 13, tailY + 5, 4, COLOR_CARD);

  // 7. Expresiones faciales (Ojos, Cejas, Boca y Efectos)
  int lx = cx - 10; // Centro ojo izquierdo
  int rx = cx + 10; // Centro ojo derecho
  int eyeY = cy + 4;

  if (emocion == GHOST_IDLE) {
    // --- OJOS IDLE: Rojos profundos con pupila y brillo blanco ---
    tft.fillCircle(lx, eyeY, 6, COLOR_DANGER);
    tft.fillCircle(rx, eyeY, 6, COLOR_DANGER);
    tft.fillCircle(lx - 1, eyeY, 3, 0x0000);
    tft.fillCircle(rx - 1, eyeY, 3, 0x0000);
    tft.drawPixel(lx + 2, eyeY - 2, ILI9341_WHITE);
    tft.drawPixel(rx + 2, eyeY - 2, ILI9341_WHITE);

    // Sonrisa apacible roja
    tft.drawLine(cx - 7, cy + 16, cx + 7, cy + 16, COLOR_DANGER);
    tft.drawPixel(cx - 8, cy + 15, COLOR_DANGER);
    tft.drawPixel(cx + 8, cy + 15, COLOR_DANGER);
    tft.drawPixel(cx, cy + 17, COLOR_DANGER);

  } else if (emocion == GHOST_THINKING) {
    // --- OJOS THINKING: Azul neón, mirada curiosa asimétrica ---
    // Ojo izquierdo normal mirando arriba/derecha
    tft.fillCircle(lx, eyeY - 1, 5, COLOR_PRIMARY);
    tft.fillCircle(lx + 1, eyeY - 2, 2, 0x0010);
    tft.drawPixel(lx + 2, eyeY - 3, ILI9341_WHITE);

    // Ojo derecho entrecerrado / inquisitivo
    tft.fillRoundRect(rx - 5, eyeY - 1, 10, 4, 2, COLOR_PRIMARY);
    tft.fillCircle(rx, eyeY - 1, 1, 0x0000);

    // Ceja derecha fruncida
    tft.drawLine(rx - 5, eyeY - 7, rx + 6, eyeY - 4, COLOR_PRIMARY);

    // Boca ondulada pensativa
    tft.drawLine(cx - 6, cy + 16, cx - 1, cy + 15, COLOR_PRIMARY);
    tft.drawLine(cx - 1, cy + 15, cx + 5, cy + 17, COLOR_PRIMARY);

    // Signo de interrogación "?" azul brillante flotante
    tft.setTextColor(COLOR_PRIMARY);
    tft.setTextSize(2);
    tft.setCursor(cx + 18, cy - 26);
    tft.print("?");

  } else if (emocion == GHOST_HAPPY) {
    // --- OJOS HAPPY: Arcos cerrados sonrientes dorados ^ ^ ---
    tft.drawLine(lx - 6, eyeY + 2, lx, eyeY - 4, COLOR_WARNING);
    tft.drawLine(lx, eyeY - 4, lx + 6, eyeY + 2, COLOR_WARNING);
    tft.drawLine(lx - 6, eyeY + 3, lx, eyeY - 3, COLOR_WARNING);

    tft.drawLine(rx - 6, eyeY + 2, rx, eyeY - 4, COLOR_WARNING);
    tft.drawLine(rx, eyeY - 4, rx + 6, eyeY + 2, COLOR_WARNING);
    tft.drawLine(rx - 6, eyeY + 3, rx, eyeY - 3, COLOR_WARNING);

    // Mejillas sonrojadas rosas
    tft.fillCircle(cx - 17, cy + 11, 3, GHOST_CHEEK_PINK);
    tft.fillCircle(cx + 17, cy + 11, 3, GHOST_CHEEK_PINK);

    // Gran boca sonriente con lengua
    tft.fillCircle(cx, cy + 16, 5, COLOR_DANGER);
    tft.fillRect(cx - 6, cy + 10, 12, 6, bodyColor); // Recorta parte superior
    tft.drawLine(cx - 5, cy + 16, cx + 5, cy + 16, COLOR_DANGER);
    tft.fillCircle(cx, cy + 18, 3, GHOST_CHEEK_PINK); // Lengua

    // Destellos / estrellas de felicidad
    tft.setTextColor(ILI9341_YELLOW);
    tft.setTextSize(1);
    tft.setCursor(cx - 24, cy - 18);
    tft.print("*");
    tft.setCursor(cx + 20, cy - 16);
    tft.print("+");

  } else if (emocion == GHOST_SURPRISED) {
    // --- OJOS SURPRISED: Grandes ojos abiertos en shock ---
    tft.fillCircle(lx, eyeY, 8, COLOR_WARNING);
    tft.drawCircle(lx, eyeY, 8, ILI9341_WHITE);
    tft.fillCircle(lx, eyeY, 4, 0x0000);
    tft.drawPixel(lx + 2, eyeY - 2, ILI9341_WHITE);

    tft.fillCircle(rx, eyeY, 8, COLOR_WARNING);
    tft.drawCircle(rx, eyeY, 8, ILI9341_WHITE);
    tft.fillCircle(rx, eyeY, 4, 0x0000);
    tft.drawPixel(rx + 2, eyeY - 2, ILI9341_WHITE);

    // Cejas levantadas de espanto
    tft.drawLine(lx - 5, eyeY - 11, lx + 5, eyeY - 13, COLOR_WARNING);
    tft.drawLine(rx - 5, eyeY - 13, rx + 5, eyeY - 11, COLOR_WARNING);

    // Boca abierta en "O"
    tft.fillCircle(cx, cy + 17, 4, 0x0000);
    tft.drawCircle(cx, cy + 17, 4, COLOR_WARNING);

    // Rayos de susto en la cabeza
    tft.drawLine(cx - 18, cy - 25, cx - 12, cy - 18, COLOR_WARNING);
    tft.drawLine(cx,      cy - 29, cx,      cy - 20, COLOR_WARNING);
    tft.drawLine(cx + 18, cy - 25, cx + 12, cy - 18, COLOR_WARNING);
  }
}

// Dibuja el marco izquierdo donde vive el Fantasma junto con su badge de estado
void dibujarPanelFantasma(GhostEmotion emocion, const char* textoBadge, uint16_t colorBadge) {
  // Contenedor tarjeta izquierda
  tft.fillRoundRect(8, 42, 102, 190, 8, COLOR_CARD);
  tft.drawRoundRect(8, 42, 102, 190, 8, COLOR_CARD_BORDER);

  // Dibujar el avatar en el centro de la tarjeta izquierda
  dibujarFantasma(59, 108, emocion);

  // Badge tipo píldora inferior con estado
  tft.fillRoundRect(14, 186, 90, 22, 6, COLOR_BG);
  tft.drawRoundRect(14, 186, 90, 22, 6, colorBadge);

  // Punto indicador de luz
  tft.fillCircle(24, 197, 3, colorBadge);

  // Texto del badge
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.setCursor(32, 194);
  tft.print(textoBadge);
}

// =========================================================================
// ESTADOS DE LA INTERFAZ DE USUARIO (UI)
// =========================================================================

void uiReposo() {
  emocionActual = GHOST_IDLE;
  tft.fillScreen(COLOR_BG);
  dibujarEncabezado("VENDING WEB3 - ESP32", COLOR_PRIMARY);

  // Panel Izquierdo: Fantasma en Modo IDLE
  dibujarPanelFantasma(GHOST_IDLE, "En linea", COLOR_SUCCESS);

  // Panel Derecho Superior: Instrucciones de selección
  tft.fillRoundRect(116, 42, 196, 120, 8, COLOR_CARD);
  tft.drawRoundRect(116, 42, 196, 120, 8, COLOR_CARD_BORDER);

  tft.setTextColor(COLOR_PRIMARY);
  tft.setTextSize(2);
  tft.setCursor(128, 56);
  tft.print("Seleccione");
  tft.setCursor(128, 76);
  tft.print("un Canal:");

  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(3);
  tft.setCursor(136, 104);
  tft.print("A1  /  A2");

  tft.setTextColor(COLOR_MUTED);
  tft.setTextSize(1);
  tft.setCursor(128, 140);
  tft.print("Marque en Teclado Pico");

  // Panel Derecho Inferior: Estado del Lector NFC
  tft.fillRoundRect(116, 168, 196, 64, 8, COLOR_CARD);
  tft.drawRoundRect(116, 168, 196, 64, 8, COLOR_CARD_BORDER);

  tft.setCursor(128, 180);
  tft.setTextColor(COLOR_SUCCESS);
  tft.setTextSize(2);
  tft.print("PN532 I2C");

  tft.setCursor(128, 204);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.print("Sensor listo y activo");
}

void uiEsperandoTarjeta(int canal) {
  emocionActual = GHOST_THINKING;
  tft.fillScreen(COLOR_BG);
  dibujarEncabezado("PAGO PENDIENTE NFC", COLOR_WARNING);

  // Panel Izquierdo: Fantasma Pensando (esperando tarjeta)
  dibujarPanelFantasma(GHOST_THINKING, "Esperando", COLOR_WARNING);

  // Panel Derecho Superior: Canal y llamada de acción
  tft.fillRoundRect(116, 42, 196, 120, 8, COLOR_CARD);
  tft.drawRoundRect(116, 42, 196, 120, 8, COLOR_WARNING);

  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(128, 54);
  tft.print("Canal:");

  tft.setTextSize(3);
  tft.setTextColor(COLOR_PRIMARY);
  tft.setCursor(205, 50);
  tft.printf("A%d", canal);

  tft.setTextColor(COLOR_WARNING);
  tft.setTextSize(2);
  tft.setCursor(128, 90);
  tft.print("Acerque");
  tft.setCursor(128, 112);
  tft.print("su Tarjeta");

  tft.setTextColor(COLOR_MUTED);
  tft.setTextSize(1);
  tft.setCursor(128, 138);
  tft.print("Tangem / Mifare NFC");

  // Panel Derecho Inferior
  tft.fillRoundRect(116, 168, 196, 64, 8, COLOR_CARD);
  tft.drawRoundRect(116, 168, 196, 64, 8, COLOR_CARD_BORDER);

  tft.setCursor(128, 180);
  tft.setTextColor(COLOR_WARNING);
  tft.setTextSize(2);
  tft.print("Escaneando...");

  tft.setCursor(128, 204);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.print("Acerquela al lector");
}

void uiTarjetaDetectada(String uidHex, int canal) {
  emocionActual = GHOST_HAPPY;
  tft.fillScreen(COLOR_BG);
  dibujarEncabezado("PAGO CONFIRMADO OK!", COLOR_SUCCESS);

  // Panel Izquierdo: Fantasma Feliz y radiante
  dibujarPanelFantasma(GHOST_HAPPY, "¡Exito!", COLOR_SUCCESS);

  // Panel Derecho Superior: UID y Despacho
  tft.fillRoundRect(116, 42, 196, 120, 8, COLOR_CARD);
  tft.drawRoundRect(116, 42, 196, 120, 8, COLOR_SUCCESS);

  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.setCursor(128, 54);
  tft.print("NFC UID DETECTADO:");

  tft.setTextColor(COLOR_SUCCESS);
  tft.setTextSize(2);
  tft.setCursor(128, 70);
  tft.print(uidHex);

  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(128, 102);
  tft.print("Despachando:");

  tft.setTextColor(COLOR_PRIMARY);
  tft.setTextSize(3);
  tft.setCursor(128, 126);
  tft.printf("Canal A%d", canal);

  // Panel Derecho Inferior
  tft.fillRoundRect(116, 168, 196, 64, 8, COLOR_CARD);
  tft.drawRoundRect(116, 168, 196, 64, 8, COLOR_CARD_BORDER);

  tft.setCursor(128, 180);
  tft.setTextColor(COLOR_SUCCESS);
  tft.setTextSize(2);
  tft.print("¡Disfrutalo!");

  tft.setCursor(128, 204);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.print("Girando servomotor...");
}

void uiErrorDrop() {
  emocionActual = GHOST_SURPRISED;
  tft.fillScreen(COLOR_BG);
  dibujarEncabezado("ALERTA SENSOR IR", COLOR_DANGER);

  // Panel Izquierdo: Fantasma Asustado / Sorprendido
  dibujarPanelFantasma(GHOST_SURPRISED, "¡Alerta!", COLOR_DANGER);

  // Panel Derecho Superior
  tft.fillRoundRect(116, 42, 196, 120, 8, COLOR_CARD);
  tft.drawRoundRect(116, 42, 196, 120, 8, COLOR_DANGER);

  tft.setTextColor(COLOR_DANGER);
  tft.setTextSize(2);
  tft.setCursor(128, 60);
  tft.print("Sin Caida");
  tft.setCursor(128, 84);
  tft.print("de Producto");

  tft.setTextColor(COLOR_MUTED);
  tft.setTextSize(1);
  tft.setCursor(128, 120);
  tft.print("Sensor IR no detecto paso");

  // Panel Derecho Inferior
  tft.fillRoundRect(116, 168, 196, 64, 8, COLOR_CARD);
  tft.drawRoundRect(116, 168, 196, 64, 8, COLOR_CARD_BORDER);

  tft.setCursor(128, 180);
  tft.setTextColor(COLOR_DANGER);
  tft.setTextSize(2);
  tft.print("Revisar Stock");

  tft.setCursor(128, 204);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.print("Verificar espiral/servo");
}

// =========================================================================
// CONFIGURACIÓN PRINCIPAL (SETUP)
// =========================================================================
void setup() {
  Serial.begin(115200);

  // 1. Inicializar Puerto Serie hacia Raspberry Pi Pico
  PicoSerial.begin(9600, SERIAL_8N1, RX2_PIN, TX2_PIN);
  Serial.println("ESP32 Master Iniciado con Avatar IAFantasma.");

  // 2. Inicializar Pantalla TFT 2.8" SPI (ILI9341)
  SPI.begin(18, 19, 23, TFT_CS); // SCK=18, MISO=19, MOSI=23, CS=5
  tft.begin(16000000); // 16 MHz para máxima estabilidad en protoboard
  tft.setRotation(3);  // 320x240 orientación horizontal
  tft.fillScreen(ILI9341_BLACK);

  // Pantalla inicial de bienvenida con el fantasma
  uiReposo();

  // 3. Inicializar Bus I2C para Lector PN532
  Wire.begin(PN532_I2C_SDA, PN532_I2C_SCL);
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("¡Error! No se halló el lector PN532 en bus I2C (0x24).");
  } else {
    Serial.print("PN532 Detectado! Firmware v");
    Serial.print((versiondata >> 16) & 0xFF, DEC);
    Serial.print('.');
    Serial.println((versiondata >> 8) & 0xFF, DEC);
    nfc.SAMConfig(); // Configuración en modo lector de tarjetas
  }
}

// =========================================================================
// BUCLE PRINCIPAL (LOOP)
// =========================================================================
void loop() {
  // A. Leer Comandos enviados por la Raspberry Pi Pico por UART2
  if (PicoSerial.available()) {
    String cmd = PicoSerial.readStringUntil('\n');
    cmd.trim();
    Serial.print("Rx Pico: ");
    Serial.println(cmd);

    if (cmd == "SELECT_1") {
      canalSeleccionado = 1;
      esperandoPago = true;
      uiEsperandoTarjeta(1);
    } 
    else if (cmd == "SELECT_2") {
      canalSeleccionado = 2;
      esperandoPago = true;
      uiEsperandoTarjeta(2);
    }
    else if (cmd.startsWith("ITEM_DROPPED")) {
      Serial.println("Pico confirma producto entregado por IR.");
      delay(2000);
      canalSeleccionado = 0;
      esperandoPago = false;
      uiReposo();
    }
    else if (cmd.startsWith("ERROR_DROP")) {
      uiErrorDrop();
      delay(3000);
      canalSeleccionado = 0;
      esperandoPago = false;
      uiReposo();
    }
  }

  // B. Comandos de prueba desde el Monitor Serie de la PC (Opcional para pruebas)
  if (Serial.available()) {
    String testCmd = Serial.readStringUntil('\n');
    testCmd.trim();
    if (testCmd == "1" || testCmd == "SELECT_1") {
      canalSeleccionado = 1;
      esperandoPago = true;
      uiEsperandoTarjeta(1);
    } else if (testCmd == "2" || testCmd == "SELECT_2") {
      canalSeleccionado = 2;
      esperandoPago = true;
      uiEsperandoTarjeta(2);
    } else if (testCmd == "HAPPY") {
      uiTarjetaDetectada("4A:8F:12:9C", 1);
    } else if (testCmd == "ERROR") {
      uiErrorDrop();
    } else if (testCmd == "IDLE") {
      uiReposo();
    }
  }

  // C. Escanear Lector NFC cuando hay un canal seleccionado
  if (esperandoPago && (millis() - ultimoEscaneo > 150)) {
    ultimoEscaneo = millis();

    uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uidLength;

    // Intenta leer la tarjeta objetivo (timeout de 100 ms)
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);

    if (success) {
      // Formatear UID a Cadena Hexadecimal (ej: 4A:8F:12:9C)
      String uidStr = "";
      for (uint8_t i = 0; i < uidLength; i++) {
        if (uid[i] < 0x10) uidStr += "0";
        uidStr += String(uid[i], HEX);
        if (i < uidLength - 1) uidStr += ":";
      }
      uidStr.toUpperCase();

      Serial.print("NFC Escaneado UID: ");
      Serial.println(uidStr);

      // Mostrar UID y confirmación con Fantasma Feliz en TFT
      uiTarjetaDetectada(uidStr, canalSeleccionado);
      delay(2000);

      // Enviar orden de giro de servomotor a la Pico por UART2
      if (canalSeleccionado == 1) {
        PicoSerial.println("PAYMENT_OK_1");
        Serial.println("Tx a Pico: PAYMENT_OK_1");
      } else if (canalSeleccionado == 2) {
        PicoSerial.println("PAYMENT_OK_2");
        Serial.println("Tx a Pico: PAYMENT_OK_2");
      }

      esperandoPago = false;
      delay(1000);
      uiReposo();
    }
  }

  delay(20);
}
