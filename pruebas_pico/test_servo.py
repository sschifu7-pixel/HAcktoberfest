"""
==============================================================================
TEST Y CALIBRACIÓN DE SERVOMOTOR EN RASPBERRY PI PICO (MicroPython)
==============================================================================
Pin por defecto: GP15 (Pin físico 20)

NOTA IMPORTANTE SOBRE "DETECCIÓN":
Los servomotores estándar (3 cables: GND, 5V, PWM) son de comunicación
UNIDIRECCIONAL (la Pico envía señal, pero el servo no devuelve datos).
No se pueden auto-detectar como un sensor I2C o SPI.
Este script realiza un ciclo de prueba y un menú interactivo para:
1. Comprobar que el pin PWM y la alimentación funcionen correctamente.
2. Calibrar el punto neutro (en servos continuos como DS04-NFC).
3. Probar giro en ambos sentidos y velocidades / ángulos.
==============================================================================
"""

from machine import Pin, PWM
import time
import sys

SERVO_PIN_NUM = 15  # GP15 (ajustable)
servo = PWM(Pin(SERVO_PIN_NUM))
servo.freq(50)  # Frecuencia estándar para servos: 50 Hz (periodo = 20 ms)

# En MicroPython con duty_u16 (rango 0 a 65535):
# Periodo = 20ms (20000us) -> 65535 ticks
# 1.0 ms (~45° o Giro rápido atrás)  -> ~3276
# 1.5 ms (90° o DETENIDO neutro)      -> ~4915
# 2.0 ms (~135° o Giro rápido adelante) -> ~6553
# Rango extendido típico: 0.5ms (~1638) a 2.5ms (~8192)

def set_pulse_ms(ms):
    """Establece el ancho de pulso en milisegundos."""
    ticks = int((ms / 20.0) * 65535)
    ticks = max(0, min(65535, ticks))
    servo.duty_u16(ticks)

def detener():
    """Punto neutro para servo continuo (1.5ms)."""
    servo.duty_u16(4915)

def test_servo_continuo():
    """Rutina para servos de 360° / rotación continua (ej: DS04-NFC de dispensador)."""
    print("\n--- INICIANDO TEST: SERVO DE ROTACIÓN CONTINUA (DS04-NFC) ---")
    
    print("[1/4] Giro ADELANTE (Sentido Horario) - 3 segundos...")
    servo.duty_u16(6000) # ~1.83 ms
    time.sleep(3)
    
    print("[2/4] PARO TOTAL (Punto Neutro: 1.5ms)...")
    detener()
    time.sleep(2)
    
    print("[3/4] Giro ATRÁS (Sentido Anti-horario) - 3 segundos...")
    servo.duty_u16(3800) # ~1.16 ms
    time.sleep(3)
    
    print("[4/4] PARO FINAL...")
    detener()
    print(">>> Test de servo continuo completado con éxito. <<<\n")

def test_servo_posicional():
    """Rutina para servos convencionales de 0° a 180° (ej: SG90, MG995, MG996R)."""
    print("\n--- INICIANDO TEST: SERVO POSICIONAL (0° - 180°) ---")
    
    posiciones = [
        ("0 grados (extremo)", 1.0),
        ("45 grados", 1.25),
        ("90 grados (centro)", 1.5),
        ("135 grados", 1.75),
        ("180 grados (extremo)", 2.0),
        ("Regreso a 90 grados", 1.5)
    ]
    
    for nombre, ms in posiciones:
        print(f"Moviendo a: {nombre} ({ms:.2f} ms)...")
        set_pulse_ms(ms)
        time.sleep(1.5)
        
    print(">>> Test posicional completado. <<<\n")

def calibrador_manual():
    """Permite ajustar el duty cycle manualmente para encontrar el punto exacto de paro."""
    print("\n--- MODO CALIBRACIÓN MANUAL ---")
    print("Ingresa valores entre 3000 y 7000 (o 'q' para salir):")
    print("Tip: 4915 suele ser el paro de fábrica en servos continuos.")
    
    while True:
        try:
            val = input("Valor duty_u16 [actual: detener=4915]: ").strip()
            if val.lower() == 'q':
                detener()
                break
            n = int(val)
            servo.duty_u16(n)
            print(f"-> Aplicado duty_u16 = {n}")
        except Exception as e:
            print("Entrada no válida:", e)

def menu():
    detener()
    while True:
        print("=" * 50)
        print("  HERRAMIENTA DE VERIFICACIÓN DE SERVO (PICO GP15)")
        print("=" * 50)
        print("1. Probar Servo Continuo (DS04-NFC / Dispensador Vending)")
        print("2. Probar Servo Posicional Estándar (0° a 180°)")
        print("3. Calibrar Punto Neutro / Manualmente")
        print("4. Detener Servo (Poner en reposo)")
        print("5. Salir")
        
        opc = input("Selecciona una opción (1-5): ").strip()
        if opc == "1":
            test_servo_continuo()
        elif opc == "2":
            test_servo_posicional()
        elif opc == "3":
            calibrador_manual()
        elif opc == "4":
            detener()
            print("Servo detenido.")
        elif opc == "5":
            detener()
            print("Saliendo...")
            break
        else:
            print("Opción no válida.")

if __name__ == "__main__":
    menu()
