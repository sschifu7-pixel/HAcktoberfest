"""
==============================================================================
TEST UNITARIO: MOTOR B (L298N) + SENSOR IR EN RASPBERRY PI PICO
==============================================================================
Conexiones requeridas:
- IN3  -> GP8  (Pin físico 11)
- IN4  -> GP7  (Pin físico 10)
- ENB  -> GP6  (Pin físico 9)   [O dejar jumper de 5V puesto en L298N]
- GND  -> GND de la Pico (Pin 3 o 38)
- VCC  -> +12V o +5V de fuente externa para el motor
- OUT3 y OUT4 -> Terminales del Motor B
- Sensor IR OUT -> GP14 (Pin físico 19)

Ejecuta este script desde Thonny para comprobar:
1. Giro del Motor B adelante (3 seg).
2. Paro del Motor B (2 seg).
3. Giro del Motor B reversa (3 seg).
4. Prueba interactiva del Sensor IR.
==============================================================================
"""

from machine import Pin, PWM
import time

PIN_IN3 = 8
PIN_IN4 = 7
PIN_ENB = 6
PIN_IR = 14

in3 = Pin(PIN_IN3, Pin.OUT)
in4 = Pin(PIN_IN4, Pin.OUT)
enb = PWM(Pin(PIN_ENB))
enb.freq(1000)

sensor_ir = Pin(PIN_IR, Pin.IN)

def motor_b_adelante(velocidad=58000):
    print(">> Motor B: ADELANTE (IN3=1, IN4=0)")
    in3.value(1)
    in4.value(0)
    enb.duty_u16(velocidad)

def motor_b_reversa(velocidad=58000):
    print(">> Motor B: REVERSA (IN3=0, IN4=1)")
    in3.value(0)
    in4.value(1)
    enb.duty_u16(velocidad)

def motor_b_parar():
    print(">> Motor B: PARADO")
    in3.value(0)
    in4.value(0)
    enb.duty_u16(0)

# Asegurar paro inicial
motor_b_parar()

print("\n==============================================")
print(" TEST DE MOTOR B CONECTADO AL L298N (PICO)")
print("==============================================")

print("\n1. Girando Motor B hacia ADELANTE durante 3 segundos...")
motor_b_adelante()
time.sleep(3)

print("2. Deteniendo motor durante 2 segundos...")
motor_b_parar()
time.sleep(2)

print("3. Girando Motor B hacia REVERSA durante 3 segundos...")
motor_b_reversa()
time.sleep(3)

print("4. Deteniendo motor...")
motor_b_parar()
time.sleep(1)

print("\n5. Test de Simulación de Despacho (Giro hasta detectar objeto en IR o 6s):")
print("   -> Pasa tu mano o un objeto frente al sensor IR en GP14 para probar...")
motor_b_adelante()

inicio = time.time()
detectado = False
while time.time() - inicio < 6:
    if sensor_ir.value() == 0:
        detectado = True
        print("   ¡¡¡ OBJETO / PRODUCTO DETECTADO POR EL SENSOR IR !!!")
        time.sleep(0.3)
        break
    time.sleep(0.05)

motor_b_parar()

if detectado:
    print("\n✅ ¡ÉXITO! El motor se detuvo al detectar el obstáculo óptico.")
else:
    print("\n⚠️ No se detectó obstáculo antes del timeout de 6 segundos.")

print("\n>>> TEST FINALIZADO CORRECTAMENTE <<<")
