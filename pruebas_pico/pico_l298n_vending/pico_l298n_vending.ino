/*
 * ==============================================================================
 * RASPBERRY PI PICO (Arduino Core) - CONTROLADOR L298N (MOTOR B) + UART ESP32
 * ==============================================================================
 * Pines:
 * - UART0: TX=GP0 (Pin 1) a ESP32 RX2 (Pin 16)
 *          RX=GP1 (Pin 2) a ESP32 TX2 (Pin 17)
 * - Motor B (L298N):
 *          IN3 = GP8  (Pin 11)
 *          IN4 = GP7  (Pin 10)
 *          ENB = GP6  (Pin 9)  [PWM]
 * - Sensor IR:
 *          OUT = GP14 (Pin 19) [Entrada Digital, 0 = Caída detectada]
 * - Joystick:
 *          VRy = GP27 / A1 (Pin 32)
 *          VRx = GP26 / A0 (Pin 31)
 *          SW  = GP16      (Pin 21, INPUT_PULLUP)
 * ==============================================================================
 */

#include <Arduino.h>

// Pines Motor B (L298N)
const int pinIN3 = 8;
const int pinIN4 = 7;
const int pinENB = 6;

// Pin Sensor IR
const int pinIR = 14;

// Joystick
const int pinJoyY = 27;
const int pinJoyBtn = 16;

const int VELOCIDAD_B = 220; // 0-255 en Arduino PWM
const unsigned long TIMEOUT_DESPACHO = 8000; // 8 segundos

bool enCentro = true;
bool botonLibre = true;

void pararMotorB() {
  digitalWrite(pinIN3, LOW);
  digitalWrite(pinIN4, LOW);
  analogWrite(pinENB, 0);
}

void girarMotorB(bool adelante = true) {
  if (adelante) {
    digitalWrite(pinIN3, HIGH);
    digitalWrite(pinIN4, LOW);
  } else {
    digitalWrite(pinIN3, LOW);
    digitalWrite(pinIN4, HIGH);
  }
  analogWrite(pinENB, VELOCIDAD_B);
}

void despacharMotorB() {
  Serial.println(">>> PAGO NFC RECIBIDO: ACTIVANDO MOTOR B <<<");
  girarMotorB(true);

  unsigned long inicio = millis();
  bool detectado = false;

  while (millis() - inicio < TIMEOUT_DESPACHO) {
    if (digitalRead(pinIR) == LOW) { // Producto cayó
      detectado = true;
      delay(350); // Margen para caída completa
      break;
    }
    delay(20);
  }

  pararMotorB();

  if (detectado) {
    Serial.println(">> Producto entregado con éxito.");
    Serial1.println("ITEM_DROPPED");
    Serial1.println("ENTREGADO");
  } else {
    Serial.println(">> Error: Timeout sin detección en sensor IR.");
    Serial1.println("ERROR_DROP");
    Serial1.println("ERROR_DISPENSA");
  }
}

void setup() {
  Serial.begin(115200);   // Monitor Serie USB
  Serial1.begin(9600);    // UART hacia ESP32 (TX=GP0, RX=GP1)

  pinMode(pinIN3, OUTPUT);
  pinMode(pinIN4, OUTPUT);
  pinMode(pinENB, OUTPUT);
  pararMotorB();

  pinMode(pinIR, INPUT);
  pinMode(pinJoyBtn, INPUT_PULLUP);

  Serial.println("Raspberry Pi Pico (Arduino Core) - L298N Motor B Listo.");
}

void loop() {
  // 1. Comandos UART desde el ESP32 tras detectar NFC
  if (Serial1.available()) {
    String msg = Serial1.readStringUntil('\n');
    msg.trim();
    Serial.print("Rx ESP32: ");
    Serial.println(msg);

    if (msg == "DESPACHAR" || msg == "PAYMENT_OK_1" || msg == "PAYMENT_OK_2" || msg == "PAYMENT_OK") {
      despacharMotorB();
    } else if (msg == "STOP") {
      pararMotorB();
    }
  }

  // 2. Comandos de prueba desde Monitor Serie USB
  if (Serial.available()) {
    String test = Serial.readStringUntil('\n');
    test.trim();
    if (test == "B" || test == "DESPACHAR" || test == "TEST") {
      despacharMotorB();
    }
  }

  // 3. Joystick hacia ESP32
  int valY = analogRead(pinJoyY);
  bool click = (digitalRead(pinJoyBtn) == LOW);

  if (valY < 300 && enCentro) {
    Serial1.println("ARRIBA");
    enCentro = false;
  } else if (valY > 800 && enCentro) {
    Serial1.println("ABAJO");
    enCentro = false;
  } else if (valY >= 400 && valY <= 700) {
    enCentro = true;
  }

  if (click && botonLibre) {
    Serial1.println("OK");
    botonLibre = false;
  } else if (!click) {
    botonLibre = true;
  }

  delay(25);
}
