/*
 * ==============================================================================
 * Test y Verificación de Servomotor en Raspberry Pi Pico (Arduino Core)
 * ==============================================================================
 * Conexión:
 * - Cable Señal (Naranja/Amarillo/Blanco): GP15 (Pin 20 de la Pico)
 * - Cable Positivo (Rojo): 5V (Pin 40 VBUS si está por USB, o fuente externa 5V)
 * - Cable Negativo (Marrón/Negro): GND (Pin 38 o cualquier GND de la Pico)
 *
 * NOTA: Los servos estándar NO envían datos de retorno (comunicación 1 vía).
 * Este sketch ejecuta un ciclo de prueba automático y permite enviar comandos
 * por el Monitor Serie para verificar que responde mecánicamente.
 * ==============================================================================
 */

#include <Servo.h>

#define SERVO_PIN 15

Servo miServo;

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n==============================================");
  Serial.println("   TEST DE SERVOMOTOR EN RASPBERRY PI PICO    ");
  Serial.println("==============================================");
  Serial.println("Pin asignado: GP15");
  Serial.println("Comandos por Monitor Serie:");
  Serial.println("  1 -> Giro Continuo / 180 grados");
  Serial.println("  0 -> Paro Neutro (1.5ms / 90 grados)");
  Serial.println("  2 -> Giro Inverso / 0 grados");
  Serial.println("  T -> Ejecutar Test Completo Automatico");
  Serial.println("==============================================\n");

  miServo.attach(SERVO_PIN);
  miServo.write(90); // Punto neutro / Detenido
  Serial.println("Servo inicializado en estado neutro (90 deg / 1.5ms).");
}

void ejecutarTestCompleto() {
  Serial.println("\n--- INICIANDO TEST AUTOMÁTICO ---");
  
  Serial.println("1. Giro Adelante (write 140)... (3 segundos)");
  miServo.write(140);
  delay(3000);

  Serial.println("2. Paro Neutro (write 90)... (2 segundos)");
  miServo.write(90);
  delay(2000);

  Serial.println("3. Giro Atras (write 40)... (3 segundos)");
  miServo.write(40);
  delay(3000);

  Serial.println("4. Paro final (write 90)...");
  miServo.write(90);
  Serial.println(">>> Fin del Test Automático <<<\n");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    if (cmd == 'T' || cmd == 't') {
      ejecutarTestCompleto();
    } else if (cmd == '1') {
      Serial.println("Comando: Giro Adelante (140)");
      miServo.write(140);
    } else if (cmd == '0') {
      Serial.println("Comando: Paro / Neutro (90)");
      miServo.write(90);
    } else if (cmd == '2') {
      Serial.println("Comando: Giro Inverso (40)");
      miServo.write(40);
    }
  }
}
