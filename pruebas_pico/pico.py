"""
==============================================================================
VENDING MACHINE WEB3 - RASPBERRY PI PICO (MicroPython)
CONTROLADOR DE DISPENSACIÓN CON PUENTE H L298N (MOTOR B) Y SENSOR IR
==============================================================================
Hardware y Conexiones:
1. Driver L298N (Motor B - Espiral dispensadora):
   - IN3  -> GP8  (Pin físico 11) -> Dirección Motor B
   - IN4  -> GP7  (Pin físico 10) -> Dirección Motor B
   - ENB  -> GP6  (Pin físico 9)  -> PWM Velocidad Motor B (o jumper 5V puesto)
   - GND  -> GND de la Pico (Pin 3 o 38) [¡TIERRA COMÚN OBLIGATORIA!]
   - VCC  -> +12V / +5V Fuente Externa para motores
   - OUT3 y OUT4 -> Conectados a los bornes del Motor B (motorreductor espiral)

2. (Opcional) Driver L298N (Motor A - Canal 1):
   - ENA  -> GP6  (Pin físico 9)   -> PWM Velocidad Motor A
   - IN1  -> GP7  (Pin físico 10)  -> Dirección Motor A
   - IN2  -> GP8  (Pin físico 11)  -> Dirección Motor A
   - OUT1 y OUT2 -> Motor A

3. Sensor Infrarrojo IR (Detección de Caída):
   - OUT  -> GP14 (Pin físico 19)  -> Entrada digital (0 = caída detectada)
   - VCC  -> 3V3 o 5V (VBUS)
   - GND  -> GND

4. UART0 (Comunicación Bidireccional con ESP32 Master):
   - TX   -> GP0  (Pin físico 1)   -> Conecta a RX2 (GPIO 16) del ESP32
   - RX   -> GP1  (Pin físico 2)   -> Conecta a TX2 (GPIO 17) del ESP32
   - GND  -> GND del ESP32

5. Joystick Analógico (Navegación en Pantalla TFT del ESP32):
   - VRy  -> GP27 / ADC1 (Pin 32)
   - VRx  -> GP26 / ADC0 (Pin 31)
   - SW   -> GP16 (Pin 21, PULL_UP)
==============================================================================
"""

from machine import ADC, Pin, PWM, UART
import time
import sys
import select

# --- CONFIGURACIÓN UART (Pico <-> ESP32 a 9600 baud) ---
uart = UART(0, baudrate=9600, tx=Pin(0), rx=Pin(1))

# --- SENSOR INFRARROJO IR (GP14) ---
# Los módulos IR típicos (ópticos / reflexivos) tienen salida activa en BAJO:
# 1 = Rayo despejado (sin caída)
# 0 = Obstáculo detectado (producto cayendo por la rampa)
sensor_ir = Pin(14, Pin.IN)

# --- CONFIGURACIÓN DRIVER L298N ---
# Pines Motor B (Conexión especificada por el usuario)
PIN_IN3 = 8   # GP8 (Pin físico 11) -> Dirección Motor B
PIN_IN4 = 7   # GP7 (Pin físico 10) -> Dirección Motor B
PIN_ENB = 6   # GP6 (Pin físico 9)  -> PWM Velocidad Motor B (o jumper 5V)

pin_in3 = Pin(PIN_IN3, Pin.OUT)
pin_in4 = Pin(PIN_IN4, Pin.OUT)
pwm_enb = PWM(Pin(PIN_ENB))
pwm_enb.freq(1000)  # Frecuencia 1 kHz para control suave de motor DC

# Pines Motor A (Canal secundario opcional)
PIN_IN1 = 7
PIN_IN2 = 8
PIN_ENA = 6

pin_in1 = Pin(PIN_IN1, Pin.OUT)
pin_in2 = Pin(PIN_IN2, Pin.OUT)
pwm_ena = PWM(Pin(PIN_ENA))
pwm_ena.freq(1000)

# (Opcional) Servomotor continuo en GP15 por compatibilidad previa
servo_pin = PWM(Pin(15))
servo_pin.freq(50)
DUTY_SERVO_PARO = 4915

# --- PARÁMETROS DE CONFIGURACIÓN ---
# Velocidad PWM (Rango 0 a 65535 ticks):
# 65535 = 100% velocidad máxima
# 58000 = ~88% velocidad recomendada para espirales
VELOCIDAD_MOTOR_B = 58000
VELOCIDAD_MOTOR_A = 58000

# Sentido de giro Motor B:
# True: IN3=1, IN4=0 (Giro horario / avance espiral)
# False: IN3=0, IN4=1 (Giro antihorario / reverso)
# ¡Cambiar a False si tu motor gira en sentido contrario al deseado!
SENTIDO_HORARIO_B = True

# Si True, cualquier orden de despacho ("DESPACHAR", "PAYMENT_OK_1", "PAYMENT_OK_2")
# activará el MOTOR B. Si False, PAYMENT_OK_1 activará Motor A y PAYMENT_OK_2 Motor B.
FORZAR_TODO_A_MOTOR_B = True

# Tiempo máximo de giro de seguridad (segundos) en caso de que el producto no caiga
TIMEOUT_DESPACHO = 8

# --- FUNCIONES DE CONTROL DE MOTORES L298N ---

def girar_motor_b(adelante=True, velocidad=VELOCIDAD_MOTOR_B):
    """Activa el Motor B en el sentido y velocidad indicados."""
    direccion = adelante if SENTIDO_HORARIO_B else (not adelante)
    if direccion:
        pin_in3.value(1)
        pin_in4.value(0)
    else:
        pin_in3.value(0)
        pin_in4.value(1)
    pwm_enb.duty_u16(velocidad)
    print(f"[MOTOR B] Activo -> Vel: {velocidad} | Sentido: {'Adelante' if direccion else 'Atras'}")

def detener_motor_b():
    """Frena y apaga el Motor B."""
    pin_in3.value(0)
    pin_in4.value(0)
    pwm_enb.duty_u16(0)
    print("[MOTOR B] Detenido.")

def girar_motor_a(adelante=True, velocidad=VELOCIDAD_MOTOR_A):
    """Activa el Motor A."""
    if adelante:
        pin_in1.value(1)
        pin_in2.value(0)
    else:
        pin_in1.value(0)
        pin_in2.value(1)
    pwm_ena.duty_u16(velocidad)
    print(f"[MOTOR A] Activo -> Vel: {velocidad}")

def detener_motor_a():
    """Frena y apaga el Motor A."""
    pin_in1.value(0)
    pin_in2.value(0)
    pwm_ena.duty_u16(0)
    print("[MOTOR A] Detenido.")

def detener_todos():
    """Detiene todos los actuadores (Motor A, Motor B y Servo)."""
    detener_motor_b()
    detener_motor_a()
    servo_pin.duty_u16(DUTY_SERVO_PARO)

# Asegurar estado de paro al arrancar
detener_todos()

# --- RUTINA DE DESPACHO CON SENSOR IR ---

def despachar_producto(motor_objetivo='B'):
    """
    Ejecuta el ciclo completo de dispensación tras la confirmación de pago NFC:
    1. Enciende el motor correspondiente (Motor B por defecto).
    2. Espera a que el sensor IR detecte la caída del producto (OUT == 0).
    3. Detiene el motor inmediatamente.
    4. Notifica el resultado al ESP32 por UART ('ITEM_DROPPED' o 'ERROR_DROP').
    """
    print("\n" + "="*50)
    print(f"🚀 INICIANDO DESPACHO CON MOTOR {motor_objetivo} (PAGO NFC OK)")
    print("="*50)

    if motor_objetivo == 'B':
        girar_motor_b(adelante=True)
    else:
        girar_motor_a(adelante=True)

    tiempo_inicio = time.time()
    producto_detectado = False

    while (time.time() - tiempo_inicio) < TIMEOUT_DESPACHO:
        # Si el sensor IR se interrumpe (caída de la golosina/producto)
        if sensor_ir.value() == 0:
            producto_detectado = True
            print("🎯 Sensor IR activado: ¡Producto detectado cayendo!")
            time.sleep(0.35)  # Margen para que termine de caer libremente
            break
        time.sleep(0.03)

    # Detener motor de inmediato
    if motor_objetivo == 'B':
        detener_motor_b()
    else:
        detener_motor_a()

    # Informar al ESP32 por UART
    if producto_detectado:
        print("✅ Despacho completado con éxito. Notificando al ESP32...")
        uart.write("ITEM_DROPPED\n")
        uart.write("ENTREGADO\n")
    else:
        print("⚠️ ALERTA: Timeout de 8s alcanzado sin detección en sensor IR.")
        uart.write("ERROR_DROP\n")
        uart.write("ERROR_DISPENSA\n")
    print("="*50 + "\n")

# --- JOYSTICK ANALÓGICO (Navegación hacia el ESP32) ---
eje_y = ADC(Pin(27))
eje_x = ADC(Pin(26))
boton = Pin(16, Pin.IN, Pin.PULL_UP)

en_centro = True
boton_libre = True

print("-------------------------------------------------------")
print(" Raspberry Pi Pico - Sistema Vending L298N Listo.")
print(" Escuchando comandos NFC del ESP32 por UART (GP0/GP1)...")
print(" Motor B asignado a: IN3=GP8, IN4=GP7, ENB=GP6")
print(" Sensor IR asignado a: GP14")
print("-------------------------------------------------------")

# --- BUCLE PRINCIPAL ---
while True:
    # 1. Leer Comandos que vienen del ESP32 por UART (tras detectar NFC)
    if uart.any():
        linea = uart.readline()
        if linea:
            msg = linea.decode('utf-8', 'ignore').strip()
            print(f"[UART Rx desde ESP32] Comando recibido: '{msg}'")

            if msg in ("DESPACHAR", "PAYMENT_OK", "DISPENSE"):
                despachar_producto(motor_objetivo='B')

            elif msg == "PAYMENT_OK_1":
                if FORZAR_TODO_A_MOTOR_B:
                    despachar_producto(motor_objetivo='B')
                else:
                    despachar_producto(motor_objetivo='A')

            elif msg == "PAYMENT_OK_2":
                despachar_producto(motor_objetivo='B')

            elif msg in ("STOP", "PARAR"):
                detener_todos()

    # 2. Comandos locales por USB Serial (Consola de Thonny / PC)
    # Permite probar Motor B directamente escribiendo 'b' o 'test'
    if select.select([sys.stdin], [], [], 0)[0]:
        cmd_pc = sys.stdin.readline().strip().upper()
        if cmd_pc in ("B", "MOTOR_B", "TEST", "DESPACHAR"):
            print("[PC Console] Prueba manual de despacho con Motor B...")
            despachar_producto(motor_objetivo='B')
        elif cmd_pc in ("A", "MOTOR_A"):
            print("[PC Console] Prueba manual de despacho con Motor A...")
            despachar_producto(motor_objetivo='A')
        elif cmd_pc in ("STOP", "0"):
            detener_todos()

    # 3. Navegación del Joystick físico hacia el ESP32
    val_y = eje_y.read_u16()
    click = (boton.value() == 0)

    if val_y < 15000 and en_centro:
        uart.write("ARRIBA\n")
        en_centro = False
    elif val_y > 50000 and en_centro:
        uart.write("ABAJO\n")
        en_centro = False
    elif 20000 < val_y < 45000:
        en_centro = True

    # Botón del Joystick (Confirmar / Pagar)
    if click and boton_libre:
        uart.write("OK\n")
        boton_libre = False
    elif not click:
        boton_libre = True

    time.sleep(0.03)