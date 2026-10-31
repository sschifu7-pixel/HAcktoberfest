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
#include <WiFi.h>
#include <WebServer.h>
#include "web_dashboard.h"

// -------------------------------------------------------------
// TIPOS Y ENUMS (AL PRINCIPIO PARA EVITAR ERROR DEL PREPROCESADOR ARDUINO)
// -------------------------------------------------------------
enum GhostEmotion : uint8_t {
  GHOST_IDLE,       // Reposo en el menú, ojos rojos clásicos y sonrisa
  GHOST_THINKING,   // Esperando pago NFC, ojos azul cielo y signo ?
  GHOST_HAPPY,      // Pago aprobado, ojos cerrados ^ ^, sonrojo y estrellas
  GHOST_SURPRISED   // Cancelado o error, ojos enormes y rayos
};

// Prototipos explícitos de funciones que usan GhostEmotion
void dibujarFantasma(int cx, int cy, GhostEmotion emocion);
void dibujarPanelFantasma(GhostEmotion emocion, const char* textoBadge, uint16_t colorBadge);

// Pines TFT (ILI9341 SPI)
#define TFT_CS   5
#define TFT_DC   4
#define TFT_RST  19  // Pin 2 para evitar conflicto con pin 19 (MISO)
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Pines PN532 I2C
#define PN532_I2C_SDA 22
#define PN532_I2C_SCL 21
Adafruit_PN532 nfc(PN532_I2C_SDA, PN532_I2C_SCL);

// Pines UART2 (comunicación con Pico)
#define RXD2 16
#define TXD2 17

// Paleta de Colores RGB565
#define COLOR_BG          ILI9341_BLACK
#define COLOR_CARD        0x18E3 // Gris grafito
#define COLOR_CARD_BORDER 0x3186 // Borde sutil
#define COLOR_PRIMARY     0x03FF // Cyan Neón
#define COLOR_SUCCESS     0x07E0 // Verde Brillante
#define COLOR_WARNING     0xFDA0 // Naranja / Ámbar
#define COLOR_DANGER      0xF800 // Rojo
#define COLOR_TEXT        ILI9341_WHITE
#define COLOR_YELLOW      0xFFE0
#define COLOR_MUTED       0x9CD3 // Gris suave

// Colores específicos para el Avatar Espectral
#define GHOST_WHITE       0xFFFF // Blanco puro
#define GHOST_ICE_BLUE    0xD7FF // Azul hielo (Thinking)
#define GHOST_GOLD_WHITE  0xFFF0 // Blanco dorado (Happy)
#define GHOST_SHOCKED     0xFFB0 // Tinte sorpresa
#define GHOST_CHEEK_PINK  0xFCB2 // Sonrojo mejillas
#define GHOST_SHADOW      0x0841 // Sombra suelo

// Definición de Productos de la Vending Machine
struct Producto {
  const char* nombre;
  const char* precio;
};

const Producto catalogo[] = {
  {"Soda Web3",      "1.50 USDC"},
  {"Cyber Snack",    "0.80 USDC"},
  {"Crypto Energy",  "2.20 USDC"}
};
const int TOTAL_PRODUCTOS = sizeof(catalogo) / sizeof(catalogo[0]);

// Máquina de estados de la Vending
enum Estado {
  MENU_SELECCION,
  ESPERANDO_PAGO,
  PAGO_EXITOSO
};

Estado estadoActual = MENU_SELECCION;
int opcionSeleccionada = 0;
bool pn532Conectado = false;

// -------------------------------------------------------------
// SERVIDOR WEB Y WI-FI ACCESS POINT (192.168.4.1)
// -------------------------------------------------------------
const char* AP_SSID = "IAFantasma_Vending";
const char* AP_PASS = "12345678"; // Min 8 chars o "" para abierto
WebServer server(80);

void dibujarMenu();
void mostrarPantallaPago();
void mostrarPantallaExito(String uidHex);

// -------------------------------------------------------------
// DIBUJO DEL AVATAR FANTASMA (Adafruit_GFX)
// -------------------------------------------------------------
void dibujarFantasma(int cx, int cy, GhostEmotion emocion) {
  uint16_t bodyColor = GHOST_WHITE;

  switch (emocion) {
    case GHOST_THINKING:  bodyColor = GHOST_ICE_BLUE;   break;
    case GHOST_HAPPY:     bodyColor = GHOST_GOLD_WHITE;  break;
    case GHOST_SURPRISED: bodyColor = GHOST_SHOCKED;     break;
    default:              bodyColor = GHOST_WHITE;       break;
  }

  // 1. Sombra bajo el fantasma
  tft.fillRoundRect(cx - 22, cy + 44, 44, 6, 3, GHOST_SHADOW);

  // 2. Anillo orbital si está pensando
  if (emocion == GHOST_THINKING) {
    tft.drawCircle(cx, cy + 10, 33, 0x0276);
    tft.drawCircle(cx, cy + 10, 34, 0x041F);
  }

  // 3. Cabeza abovedada (Cúpula)
  int r = 25;
  tft.fillCircle(cx, cy, r, bodyColor);

  // 4. Torso principal
  tft.fillRect(cx - r, cy, r * 2, 32, bodyColor);

  // 5. Destello de brillo superior izquierdo (estilo 3D)
  tft.fillCircle(cx - 10, cy - 11, 4, ILI9341_WHITE);

  // 6. Pliegues ondulados inferiores (cola del fantasma)
  int tailY = cy + 32;
  tft.fillCircle(cx - 18, tailY, 6, bodyColor);
  tft.fillCircle(cx - 6,  tailY, 6, bodyColor);
  tft.fillCircle(cx + 6,  tailY, 6, bodyColor);
  tft.fillCircle(cx + 18, tailY, 6, bodyColor);

  // Recortes para dar efecto de ondas
  tft.fillCircle(cx - 12, tailY + 5, 4, COLOR_CARD);
  tft.fillCircle(cx,      tailY + 5, 4, COLOR_CARD);
  tft.fillCircle(cx + 12, tailY + 5, 4, COLOR_CARD);

  // 7. Expresiones faciales según la emoción
  int lx = cx - 10; // Centro ojo izquierdo
  int rx = cx + 10; // Centro ojo derecho
  int eyeY = cy + 4;

  if (emocion == GHOST_IDLE) {
    // --- OJOS IDLE: Rojos espectrales con pupila negra y brillo blanco ---
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

    // Signo de interrogación "?" azul flotante
    tft.setTextColor(COLOR_PRIMARY);
    tft.setTextSize(2);
    tft.setCursor(cx + 17, cy - 25);
    tft.print("?");

  } else if (emocion == GHOST_HAPPY) {
    // --- OJOS HAPPY: Arcos sonrientes dorados ^ ^ ---
    tft.drawLine(lx - 6, eyeY + 2, lx, eyeY - 4, COLOR_WARNING);
    tft.drawLine(lx, eyeY - 4, lx + 6, eyeY + 2, COLOR_WARNING);
    tft.drawLine(lx - 6, eyeY + 3, lx, eyeY - 3, COLOR_WARNING);

    tft.drawLine(rx - 6, eyeY + 2, rx, eyeY - 4, COLOR_WARNING);
    tft.drawLine(rx, eyeY - 4, rx + 6, eyeY + 2, COLOR_WARNING);
    tft.drawLine(rx - 6, eyeY + 3, rx, eyeY - 3, COLOR_WARNING);

    // Mejillas sonrojadas rosas
    tft.fillCircle(cx - 16, cy + 11, 3, GHOST_CHEEK_PINK);
    tft.fillCircle(cx + 16, cy + 11, 3, GHOST_CHEEK_PINK);

    // Gran boca feliz con lengua
    tft.fillCircle(cx, cy + 16, 5, COLOR_DANGER);
    tft.fillRect(cx - 6, cy + 10, 12, 6, bodyColor);
    tft.drawLine(cx - 5, cy + 16, cx + 5, cy + 16, COLOR_DANGER);
    tft.fillCircle(cx, cy + 18, 3, GHOST_CHEEK_PINK); // Lengua

    // Destellos / estrellas
    tft.setTextColor(ILI9341_YELLOW);
    tft.setTextSize(1);
    tft.setCursor(cx - 23, cy - 17);
    tft.print("*");
    tft.setCursor(cx + 19, cy - 15);
    tft.print("+");

  } else if (emocion == GHOST_SURPRISED) {
    // --- OJOS SURPRISED: Ojos abiertos gigantescos ---
    tft.fillCircle(lx, eyeY, 8, COLOR_WARNING);
    tft.drawCircle(lx, eyeY, 8, ILI9341_WHITE);
    tft.fillCircle(lx, eyeY, 4, 0x0000);
    tft.drawPixel(lx + 2, eyeY - 2, ILI9341_WHITE);

    tft.fillCircle(rx, eyeY, 8, COLOR_WARNING);
    tft.drawCircle(rx, eyeY, 8, ILI9341_WHITE);
    tft.fillCircle(rx, eyeY, 4, 0x0000);
    tft.drawPixel(rx + 2, eyeY - 2, ILI9341_WHITE);

    // Cejas levantadas
    tft.drawLine(lx - 5, eyeY - 11, lx + 5, eyeY - 13, COLOR_WARNING);
    tft.drawLine(rx - 5, eyeY - 13, rx + 5, eyeY - 11, COLOR_WARNING);

    // Boca en "O"
    tft.fillCircle(cx, cy + 17, 4, 0x0000);
    tft.drawCircle(cx, cy + 17, 4, COLOR_WARNING);

    // Rayos de susto
    tft.drawLine(cx - 18, cy - 25, cx - 12, cy - 18, COLOR_WARNING);
    tft.drawLine(cx,      cy - 29, cx,      cy - 20, COLOR_WARNING);
    tft.drawLine(cx + 18, cy - 25, cx + 12, cy - 18, COLOR_WARNING);
  }
}

// Dibuja el marco izquierdo donde vive el Fantasma junto con su badge
void dibujarPanelFantasma(GhostEmotion emocion, const char* textoBadge, uint16_t colorBadge) {
  tft.fillRoundRect(6, 38, 98, 196, 8, COLOR_CARD);
  tft.drawRoundRect(6, 38, 98, 196, 8, COLOR_CARD_BORDER);

  dibujarFantasma(55, 102, emocion);

  tft.fillRoundRect(12, 184, 86, 22, 6, COLOR_BG);
  tft.drawRoundRect(12, 184, 86, 22, 6, colorBadge);

  tft.fillCircle(21, 195, 3, colorBadge);

  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.setCursor(29, 192);
  tft.print(textoBadge);
}

// -------------------------------------------------------------
// FUNCIONES DE DIBUJO DE INTERFAZ (UI - MODO HORIZONTAL 320x240)
// -------------------------------------------------------------
void dibujarCabecera(const char* titulo, uint16_t color) {
  tft.fillRect(0, 0, 320, 32, color);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(12, 8);
  tft.print(titulo);
}

void dibujarMenu() {
  tft.fillScreen(COLOR_BG);
  dibujarCabecera("VENDING MACHINE WEB3", COLOR_PRIMARY);

  // Panel Izquierdo: Fantasma en Reposo
  dibujarPanelFantasma(GHOST_IDLE, "En linea", COLOR_SUCCESS);

  // Panel Derecho: Lista de 3 Productos del Catálogo
  int yBase = 38;
  int altoCard = 48;
  int espaciado = 6;

  for (int i = 0; i < TOTAL_PRODUCTOS; i++) {
    int yPos = yBase + (i * (altoCard + espaciado));

    if (i == opcionSeleccionada) {
      tft.fillRoundRect(110, yPos, 204, altoCard, 6, COLOR_PRIMARY);
      tft.drawRoundRect(110, yPos, 204, altoCard, 6, ILI9341_WHITE);
      tft.setTextColor(ILI9341_BLACK);
    } else {
      tft.fillRoundRect(110, yPos, 204, altoCard, 6, COLOR_CARD);
      tft.drawRoundRect(110, yPos, 204, altoCard, 6, COLOR_CARD_BORDER);
      tft.setTextColor(COLOR_TEXT);
    }

    tft.setTextSize(2);
    tft.setCursor(120, yPos + 8);
    tft.print(catalogo[i].nombre);

    tft.setTextSize(1);
    tft.setCursor(120, yPos + 30);
    tft.print("Precio: ");
    tft.print(catalogo[i].precio);
  }

  // Barra informativa inferior derecha
  tft.fillRoundRect(110, 200, 204, 34, 6, COLOR_CARD);
  tft.drawRoundRect(110, 200, 204, 34, 6, COLOR_CARD_BORDER);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(118, 206);
  tft.print("Joystick: Mover / OK");

  tft.setTextColor(COLOR_MUTED);
  tft.setCursor(118, 218);
  tft.print("Elige producto a pagar");
}

void mostrarPantallaPago() {
  tft.fillScreen(COLOR_BG);
  dibujarCabecera("PROCESO DE PAGO", COLOR_WARNING);

  // Panel Izquierdo: Fantasma Pensando (esperando tarjeta)
  dibujarPanelFantasma(GHOST_THINKING, "Esperando", COLOR_WARNING);

  // Panel Derecho Superior: Producto seleccionado y Total
  tft.fillRoundRect(110, 38, 204, 82, 8, COLOR_CARD);
  tft.drawRoundRect(110, 38, 204, 82, 8, COLOR_WARNING);

  tft.setTextSize(2);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(120, 50);
  tft.print(catalogo[opcionSeleccionada].nombre);

  tft.setTextColor(COLOR_PRIMARY);
  tft.setCursor(120, 72);
  tft.print("Total: ");

  tft.setTextColor(COLOR_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(120, 94);
  tft.print(catalogo[opcionSeleccionada].precio);

  // Panel Derecho Inferior: Instrucciones de NFC
  tft.fillRoundRect(110, 126, 204, 108, 8, COLOR_CARD);
  tft.drawRoundRect(110, 126, 204, 108, 8, COLOR_SUCCESS);

  tft.setTextSize(2);
  tft.setTextColor(COLOR_SUCCESS);
  tft.setCursor(120, 138);
  tft.print("Acerque Tag");
  tft.setCursor(120, 158);
  tft.print("o Tarjeta");

  tft.setTextSize(1);
  tft.setTextColor(COLOR_MUTED);
  tft.setCursor(120, 184);
  tft.print("Tangem / Mifare NFC");

  tft.setTextColor(COLOR_DANGER);
  tft.setCursor(120, 208);
  tft.print("Presione OK: Cancelar");
}

void mostrarPantallaExito(String uidHex) {
  tft.fillScreen(COLOR_BG);
  dibujarCabecera("PAGO CONFIRMADO OK!", COLOR_SUCCESS);

  // Panel Izquierdo: Fantasma Feliz y radiante
  dibujarPanelFantasma(GHOST_HAPPY, "¡Exito!", COLOR_SUCCESS);

  // Panel Derecho Superior: Producto y UID
  tft.fillRoundRect(110, 38, 204, 108, 8, COLOR_CARD);
  tft.drawRoundRect(110, 38, 204, 108, 8, COLOR_SUCCESS);

  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(120, 50);
  tft.print("Despachando:");

  tft.setTextColor(COLOR_PRIMARY);
  tft.setTextSize(2);
  tft.setCursor(120, 74);
  tft.print(catalogo[opcionSeleccionada].nombre);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(120, 104);
  tft.print("Card UID: ");
  tft.print(uidHex);

  tft.setCursor(120, 122);
  tft.setTextColor(COLOR_SUCCESS);
  tft.print("Transaccion aprobada!");

  // Panel Derecho Inferior: Agradecimiento
  tft.fillRoundRect(110, 152, 204, 82, 8, COLOR_CARD);
  tft.drawRoundRect(110, 152, 204, 82, 8, COLOR_CARD_BORDER);

  tft.setTextColor(COLOR_SUCCESS);
  tft.setTextSize(2);
  tft.setCursor(120, 168);
  tft.print("¡GRACIAS!");

  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.setCursor(120, 198);
  tft.print("Retire su producto");
}

// -------------------------------------------------------------
// SETUP Y RUTAS DEL SERVIDOR WEB
// -------------------------------------------------------------
String obtenerEstadoString() {
  switch (estadoActual) {
    case ESPERANDO_PAGO: return "ESPERANDO_PAGO";
    case PAGO_EXITOSO:   return "PAGO_EXITOSO";
    default:             return "MENU_SELECCION";
  }
}

void procesarComandoWeb(String cmd) {
  cmd.trim();
  Serial.print("Web Cmd: ");
  Serial.println(cmd);

  if (cmd == "ARRIBA" || cmd == "UP") {
    opcionSeleccionada--;
    if (opcionSeleccionada < 0) opcionSeleccionada = TOTAL_PRODUCTOS - 1;
    if (estadoActual == MENU_SELECCION) dibujarMenu();
  }
  else if (cmd == "ABAJO" || cmd == "DOWN") {
    opcionSeleccionada++;
    if (opcionSeleccionada >= TOTAL_PRODUCTOS) opcionSeleccionada = 0;
    if (estadoActual == MENU_SELECCION) dibujarMenu();
  }
  else if (cmd.startsWith("SELECT:")) {
    int idx = cmd.substring(7).toInt();
    if (idx >= 0 && idx < TOTAL_PRODUCTOS) {
      opcionSeleccionada = idx;
      if (estadoActual == MENU_SELECCION) dibujarMenu();
    }
  }
  else if (cmd == "OK") {
    if (estadoActual == MENU_SELECCION) {
      estadoActual = ESPERANDO_PAGO;
      mostrarPantallaPago();
    } else {
      estadoActual = MENU_SELECCION;
      dibujarMenu();
    }
  }
  else if (cmd.startsWith("NFC_TAP:")) {
    String uidHex = cmd.substring(8);
    if (uidHex.length() == 0) uidHex = "4A:8F:12:9C";
    estadoActual = PAGO_EXITOSO;
    mostrarPantallaExito(uidHex);
    Serial2.println("DESPACHAR");
    Serial.println("Tx Pico: DESPACHAR");
    delay(3500);
    estadoActual = MENU_SELECCION;
    dibujarMenu();
  }
  else if (cmd == "DESPACHAR") {
    Serial2.println("DESPACHAR");
    Serial.println("Tx Pico: DESPACHAR");
  }
  else if (cmd == "STOP_SERVO") {
    Serial2.println("STOP_SERVO");
    Serial.println("Tx Pico: STOP_SERVO");
  }
  else if (cmd == "ITEM_DROPPED") {
    Serial.println("Rx Web: ITEM_DROPPED (Caída de producto)");
  }
  else if (cmd == "ERROR_DROP") {
    Serial.println("Rx Web: ERROR_DROP (Timeout)");
  }
}

void setupWebServer() {
  // Página Principal HTML
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", PAGE_INDEX_HTML);
  });

  // API para consultar el estado actual en JSON
  server.on("/status", HTTP_GET, []() {
    String json = "{\"state\":\"" + obtenerEstadoString() + "\",\"selected\":" + String(opcionSeleccionada) + "}";
    server.send(200, "application/json", json);
  });

  // API para recibir comandos desde la web
  server.on("/api/cmd", HTTP_POST, []() {
    if (server.hasArg("c")) {
      procesarComandoWeb(server.arg("c"));
    }
    String json = "{\"state\":\"" + obtenerEstadoString() + "\",\"selected\":" + String(opcionSeleccionada) + "}";
    server.send(200, "application/json", json);
  });

  // Server-Sent Events (SSE) para actualización en tiempo real
  server.on("/events", HTTP_GET, []() {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/event-stream", "");
    server.sendHeader("Cache-Control", "no-cache");
    server.sendHeader("Connection", "keep-alive");
    server.sendContent("data: STATE:" + obtenerEstadoString() + ":" + String(opcionSeleccionada) + "\n\n");
  });

  server.begin();
  Serial.println("[OK] Servidor Web activo en http://192.168.4.1");
}

// -------------------------------------------------------------
// SETUP
// -------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  // Inicialización de UART2 hacia la Pico
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println("ESP32 Vending Machine Iniciada con Avatar IAFantasma.");

  // Configurar Wi-Fi en modo Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Punto de Acceso Wi-Fi creado: ");
  Serial.println(AP_SSID);
  Serial.print("Conectate a esa red y abre en el navegador: http://");
  Serial.println(IP);

  // Iniciar servidor web
  setupWebServer();

  // Inicializar bus SPI a 16 MHz para eliminar ruido/estática
  SPI.begin(18, 19, 23, TFT_CS); // SCK=18, MISO=19, MOSI=23, CS=5
  tft.begin(16000000);            // 16 MHz para estabilidad máxima con cables jumper
  
  // Orientación Horizontal 320x240
  tft.setRotation(0);
  tft.fillScreen(COLOR_BG);

  // Inicializar Bus I2C para PN532
  Wire.begin(PN532_I2C_SDA, PN532_I2C_SCL);
  nfc.begin();
  uint32_t version = nfc.getFirmwareVersion();
  if (version) {
    pn532Conectado = true;
    nfc.SAMConfig();
    Serial.println("[OK] Lector PN532 configurado.");
  } else {
    Serial.println("[ERROR] PN532 no detectado en bus I2C (0x24).");
  }

  // Pintar interfaz completa (320x240)
  dibujarMenu();
}

// -------------------------------------------------------------
// LOOP PRINCIPAL
// -------------------------------------------------------------
void loop() {
  // 0. Atender peticiones del servidor web (Panel de Control)
  server.handleClient();

  // 1. Lectura de comandos enviados por la Pico mediante UART2
  if (Serial2.available()) {
    String comando = Serial2.readStringUntil('\n');
    comando.trim();
    Serial.print("Rx Pico: ");
    Serial.println(comando);

    if (estadoActual == MENU_SELECCION) {
      if (comando == "ARRIBA" || comando == "UP") {
        opcionSeleccionada--;
        if (opcionSeleccionada < 0) opcionSeleccionada = TOTAL_PRODUCTOS - 1;
        dibujarMenu();
      } 
      else if (comando == "ABAJO" || comando == "DOWN") {
        opcionSeleccionada++;
        if (opcionSeleccionada >= TOTAL_PRODUCTOS) opcionSeleccionada = 0;
        dibujarMenu();
      } 
      else if (comando == "OK") {
        estadoActual = ESPERANDO_PAGO;
        mostrarPantallaPago();
      }
    } 
    else if (estadoActual == ESPERANDO_PAGO) {
      if (comando == "OK" || comando == "CANCEL") {
        estadoActual = MENU_SELECCION;
        dibujarMenu();
      }
    }
  }

  // 1.B. Comandos de prueba desde el Monitor Serie de la PC
  if (Serial.available()) {
    String testCmd = Serial.readStringUntil('\n');
    testCmd.trim();
    if (testCmd == "ARRIBA" || testCmd == "w") {
      opcionSeleccionada = (opcionSeleccionada - 1 + TOTAL_PRODUCTOS) % TOTAL_PRODUCTOS;
      dibujarMenu();
    } else if (testCmd == "ABAJO" || testCmd == "s") {
      opcionSeleccionada = (opcionSeleccionada + 1) % TOTAL_PRODUCTOS;
      dibujarMenu();
    } else if (testCmd == "OK" || testCmd == " ") {
      if (estadoActual == MENU_SELECCION) {
        estadoActual = ESPERANDO_PAGO;
        mostrarPantallaPago();
      } else {
        estadoActual = MENU_SELECCION;
        dibujarMenu();
      }
    } else if (testCmd == "HAPPY") {
      mostrarPantallaExito("4A:8F:12:9C");
    }
  }

  // 2. Lectura de NFC solo cuando se está esperando pago
  if (estadoActual == ESPERANDO_PAGO && pn532Conectado) {
    uint8_t success;
    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
    uint8_t uidLength = 0;

    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 80);

    if (success) {
      String uidHex = "";
      for (uint8_t i = 0; i < uidLength; i++) {
        if (uid[i] < 0x10) uidHex += "0";
        uidHex += String(uid[i], HEX);
        if (i < uidLength - 1) uidHex += ":";
      }
      uidHex.toUpperCase();

      estadoActual = PAGO_EXITOSO;
      mostrarPantallaExito(uidHex);

      Serial2.println("DESPACHAR"); 
      Serial.println("Tx Pico: DESPACHAR");

      delay(3500);

      estadoActual = MENU_SELECCION;
      dibujarMenu();
    }
  }

  delay(10);
}