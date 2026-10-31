"""
==============================================================================
SERVIDOR LOCAL MOCK DE n8n (Flujo de Información en Tiempo Real)
==============================================================================
Este script simula el webhook de n8n en el puerto 5678 (http://localhost:5678)
mientras se instala n8n oficial o si quieres una demo rápida sin dependencias.

Escucha en:
POST http://localhost:5678/webhook/vending-order

Para ejecutar:
python mock_n8n_server.py
==============================================================================
"""

import http.server
import socketserver
import json
from datetime import datetime

PORT = 5678

class N8nWebhookHandler(http.server.SimpleHTTPRequestHandler):
    def do_OPTIONS(self):
        # Soporte CORS para llamadas desde el navegador (Angular / ESP32)
        self.send_response(200)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type, xi-api-key')
        self.end_headers()

    def do_POST(self):
        content_length = int(self.headers.get('Content-Length', 0))
        post_data = self.rfile.read(content_length)

        try:
            payload = json.loads(post_data.decode('utf-8'))
        except Exception:
            payload = {"raw": post_data.decode('utf-8', 'ignore')}

        hora = datetime.now().strftime('%H:%M:%S')
        evento = payload.get('event', 'DESCONOCIDO')

        print("\n" + "="*60)
        print(f"⚡ [n8n WORKFLOW RECIBIDO] - {hora} | Evento: {evento}")
        print("="*60)

        if evento == "VOICE_ORDER_RECEIVED":
            prod = payload.get('product', {})
            frase = payload.get('phrase', '')
            print(f"🎙️ ORDEN POR VOZ: \"{frase}\"")
            print(f"📦 Producto Solicitado: {prod.get('nombre', 'N/A')} ({prod.get('precio', '')})")
            print("🚀 Acción: Esperando tarjeta NFC para autorizar...")

        elif evento == "NFC_PAYMENT_CONFIRMED":
            uid = payload.get('card_uid', '')
            prod = payload.get('product', {})
            monto = payload.get('amount', '')
            print(f"💳 PAGO NFC CONFIRMADO: Tarjeta UID [{uid}]")
            print(f"💰 Monto cobrado: {monto}")
            print(f"🔄 Motor Activado: Motor B (L298N en Raspberry Pi Pico)")

        elif evento == "DISPENSE_SUCCESS_IR_DETECTED":
            print("✅ ENTREGA EXITOSA: Sensor óptico IR (GP14) detectó la caída del producto.")
            print("🛑 Motor B detenido.")

        elif evento == "DISPENSE_TIMEOUT_ALERT":
            print("⚠️ ALERTA: Timeout en sensor IR. Posible atasco de espiral o falta de stock.")

        elif evento == "TEST_CONNECTION":
            print(f"🧪 PRUEBA DE CONEXIÓN: {payload.get('message', 'OK')}")

        else:
            print(json.dumps(payload, indent=2, ensure_ascii=False))

        print("="*60)

        # Responder a Angular / ESP32
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()

        response = {
            "status": "success",
            "message": f"Evento {evento} procesado en n8n",
            "timestamp": datetime.now().isoformat()
        }
        self.wfile.write(json.dumps(response).encode('utf-8'))

    def log_message(self, format, *args):
        # Silenciar logs HTTP por defecto para mantener limpia la consola
        return

if __name__ == '__main__':
    # Permitir reutilizar la dirección rápidamente si se reinicia
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", PORT), N8nWebhookHandler) as httpd:
        print("------------------------------------------------------------------")
        print(f"  ⚡ SERVIDOR LOCAL n8n WEBHOOK ACTIVO EN PUERTO {PORT}")
        print(f"  Endpoint: http://localhost:{PORT}/webhook/vending-order")
        print("  Escuchando eventos de voz, pagos NFC y telemetría de la máquina...")
        print("------------------------------------------------------------------")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nServidor n8n detenido.")
