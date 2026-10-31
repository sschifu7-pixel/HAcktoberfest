import { Injectable, signal, computed } from '@angular/core';

export type GhostEmotion = 'idle' | 'thinking' | 'happy' | 'surprised';
export type EstadoVending = 'MENU_SELECCION' | 'ESPERANDO_PAGO' | 'PAGO_EXITOSO';

export interface VendingProduct {
  id: number;
  nombre: string;
  precio: string;
  descripcion: string;
  icon: string;
  color: string;
  stock: number;
}

export interface SerialLog {
  direction: 'rx' | 'tx' | 'sys';
  text: string;
  time: string;
}

@Injectable({
  providedIn: 'root'
})
export class VendingService {
  // Estado de la Vending Machine (Sincronizado con esp32_vending_i2c_nfc.ino)
  estadoActual = signal<EstadoVending>('MENU_SELECCION');
  opcionSeleccionada = signal<number>(0);
  ghostEmotion = signal<GhostEmotion>('idle');
  ghostSpeech = signal<string>('¡Bienvenido! Selecciona un producto del catálogo con el joystick o en pantalla.');
  lastScannedUid = signal<string>('4A:8F:12:9C');

  // Catálogo de Productos exacto de esp32_vending_i2c_nfc.ino
  catalogo = signal<VendingProduct[]>([
    {
      id: 0,
      nombre: 'Soda Web3',
      precio: '1.50 USDC',
      descripcion: 'Refresco gasificado descentralizado con infusión de menta y burbujas cuánticas.',
      icon: '🥤',
      color: '#00f2fe',
      stock: 5
    },
    {
      id: 1,
      nombre: 'Cyber Snack',
      precio: '0.80 USDC',
      descripcion: 'Barra energética con sabor a chocolate espectral para recargar energía.',
      icon: '🍫',
      color: '#ffb700',
      stock: 6
    },
    {
      id: 2,
      nombre: 'Crypto Energy',
      precio: '2.20 USDC',
      descripcion: 'Bebida de alto voltaje y cafeína cuántica para hackathones intensos.',
      icon: '⚡',
      color: '#ff007f',
      stock: 4
    }
  ]);

  productoActual = computed(() => {
    const idx = this.opcionSeleccionada();
    const list = this.catalogo();
    return list[idx] || list[0];
  });

  // Telemetría de Servomotor DS04-NFC y Sensor IR (Raspberry Pi Pico)
  servoRunning = signal<boolean>(false);
  servoDirectionInverted = signal<boolean>(true); // Duty 3800 (sentido contrario configurado en pico.py)
  servoDuty = signal<number>(4915); // 4915 = detenido
  irTriggered = signal<boolean>(false); // Sensor IR detectando caída (GP14)
  pn532HardwareOk = signal<boolean>(false);

  // Conexión Web Serial API (USB directo a ESP32 o Pico)
  serialConnected = signal<boolean>(false);
  serialPortName = signal<string>('Desconectado');
  serialLogs = signal<SerialLog[]>([]);
  private port: any = null;
  private reader: any = null;
  private readLoopActive = false;

  // Conexión Wi-Fi / WebSocket (Inalámbrico al ESP32)
  wifiConnected = signal<boolean>(false);
  esp32Ip = signal<string>('192.168.4.1'); // IP por defecto en modo AP
  private ws: WebSocket | null = null;

  // Sonido Sintetizado Web Audio API
  soundEnabled = signal<boolean>(true);
  private audioCtx: AudioContext | null = null;

  constructor() {
    this.addLog('sys', 'IAFantasma Web3 Vending sincronizado con ESP32.');
  }

  // --- AUDIO SYNTHESIZER ---
  private initAudio() {
    if (!this.audioCtx) {
      const AudioCtx = window.AudioContext || (window as any).webkitAudioContext;
      if (AudioCtx) this.audioCtx = new AudioCtx();
    }
    if (this.audioCtx && this.audioCtx.state === 'suspended') {
      this.audioCtx.resume();
    }
  }

  playTone(freq: number, type: OscillatorType = 'sine', duration = 0.12, vol = 0.2) {
    if (!this.soundEnabled()) return;
    try {
      this.initAudio();
      if (!this.audioCtx) return;
      const osc = this.audioCtx.createOscillator();
      const gain = this.audioCtx.createGain();
      osc.type = type;
      osc.frequency.setValueAtTime(freq, this.audioCtx.currentTime);
      gain.gain.setValueAtTime(vol, this.audioCtx.currentTime);
      gain.gain.exponentialRampToValueAtTime(0.001, this.audioCtx.currentTime + duration);
      osc.connect(gain);
      gain.connect(this.audioCtx.destination);
      osc.start();
      osc.stop(this.audioCtx.currentTime + duration);
    } catch {}
  }

  playSuccessSound() {
    this.playTone(880, 'sine', 0.1, 0.2);
    setTimeout(() => this.playTone(1320, 'sine', 0.2, 0.2), 90);
    setTimeout(() => this.playTone(1760, 'triangle', 0.35, 0.2), 200);
  }

  playErrorSound() {
    this.playTone(280, 'sawtooth', 0.15, 0.25);
    setTimeout(() => this.playTone(200, 'sawtooth', 0.25, 0.25), 150);
  }

  playClickSound() {
    this.playTone(1200, 'triangle', 0.04, 0.1);
  }

  // --- LOGS ---
  addLog(direction: 'rx' | 'tx' | 'sys', text: string) {
    const time = new Date().toLocaleTimeString();
    this.serialLogs.update(logs => [{ direction, text, time }, ...logs.slice(0, 49)]);
  }

  clearLogs() {
    this.serialLogs.set([]);
  }

  // --- WEB SERIAL API (Conexión USB con ESP32 / Pico a 115200) ---
  isSerialSupported(): boolean {
    return 'serial' in navigator;
  }

  async connectSerial() {
    if (!this.isSerialSupported()) {
      alert('Tu navegador no soporta Web Serial API. Usa Google Chrome o Microsoft Edge.');
      return;
    }
    try {
      this.port = await (navigator as any).serial.requestPort();
      await this.port.open({ baudRate: 115200 }); // Baudrate oficial del ESP32 en esp32_vending_i2c_nfc.ino
      this.serialConnected.set(true);
      this.serialPortName.set('ESP32 Conectado (115200 baud)');
      this.addLog('sys', 'Puerto USB abierto a 115200 baudios.');
      this.playSuccessSound();
      this.startReadingSerial();
    } catch (err: any) {
      this.addLog('sys', `Error al conectar puerto USB: ${err?.message || err}`);
      this.serialConnected.set(false);
    }
  }

  async disconnectSerial() {
    this.readLoopActive = false;
    try {
      if (this.reader) await this.reader.cancel();
      if (this.port) await this.port.close();
    } catch {}
    this.port = null;
    this.reader = null;
    this.serialConnected.set(false);
    this.serialPortName.set('Desconectado');
    this.addLog('sys', 'Puerto USB cerrado.');
  }

  private async startReadingSerial() {
    this.readLoopActive = true;
    const textDecoder = new TextDecoderStream();
    this.port.readable.pipeTo(textDecoder.writable);
    this.reader = textDecoder.readable.getReader();

    let buffer = '';
    try {
      while (this.readLoopActive) {
        const { value, done } = await this.reader.read();
        if (done) break;
        if (value) {
          buffer += value;
          const lines = buffer.split('\n');
          buffer = lines.pop() || '';
          for (const line of lines) {
            const trimmed = line.trim();
            if (trimmed) {
              this.handleSerialIncoming(trimmed);
            }
          }
        }
      }
    } catch (err) {
      if (this.readLoopActive) {
        this.addLog('sys', `Lectura USB interrumpida: ${err}`);
      }
    }
  }

  // --- CONEXIÓN WI-FI / WEBSOCKET ---
  connectWifi(ip?: string) {
    const targetIp = ip || this.esp32Ip();
    this.addLog('sys', `Intentando conectar por Wi-Fi a ws://${targetIp}:81...`);
    try {
      this.ws = new WebSocket(`ws://${targetIp}:81`);
      
      this.ws.onopen = () => {
        this.wifiConnected.set(true);
        this.addLog('sys', `¡Conexión Wi-Fi establecida con ESP32 en ${targetIp}!`);
        this.playSuccessSound();
      };

      this.ws.onmessage = (event) => {
        const data = event.data?.toString().trim();
        if (data) {
          this.handleSerialIncoming(data);
        }
      };

      this.ws.onerror = (err) => {
        this.addLog('sys', `Error en conexión Wi-Fi con ESP32.`);
        this.wifiConnected.set(false);
      };

      this.ws.onclose = () => {
        this.wifiConnected.set(false);
        this.addLog('sys', 'Conexión Wi-Fi cerrada.');
      };
    } catch (e: any) {
      this.addLog('sys', `Fallo al iniciar WebSocket: ${e?.message || e}`);
    }
  }

  disconnectWifi() {
    if (this.ws) {
      this.ws.close();
      this.ws = null;
    }
    this.wifiConnected.set(false);
    this.addLog('sys', 'Desconectado de Wi-Fi.');
  }

  async sendSerial(command: string) {
    this.addLog('tx', command);
    
    // Si está conectado por USB Serial
    if (this.port && this.port.writable) {
      try {
        const textEncoder = new TextEncoder();
        const writer = this.port.writable.getWriter();
        await writer.write(textEncoder.encode(command + '\n'));
        writer.releaseLock();
      } catch (err) {
        this.addLog('sys', `Error al enviar trama USB: ${err}`);
      }
    }

    // Si está conectado por Wi-Fi WebSocket
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      try {
        this.ws.send(command + '\n');
      } catch (err) {
        this.addLog('sys', `Error al enviar trama Wi-Fi: ${err}`);
      }
    }
  }

  // Interpretar tramas que envía el ESP32
  handleSerialIncoming(line: string) {
    this.addLog('rx', line);

    // Estado del hardware
    if (line.includes('[OK] Lector PN532 configurado')) {
      this.pn532HardwareOk.set(true);
    }

    // Comandos de joystick recibidos por el ESP32
    if (line.includes('Rx Pico: ARRIBA') || line.includes('Rx Pico: UP')) {
      this.selectPreviousProduct(false);
    } else if (line.includes('Rx Pico: ABAJO') || line.includes('Rx Pico: DOWN')) {
      this.selectNextProduct(false);
    } else if (line.includes('Rx Pico: OK')) {
      this.pressOkButton(false);
    }

    // Orden de despacho enviada a la Pico
    if (line.includes('Tx Pico: DESPACHAR') || line.includes('DESPACHAR')) {
      this.triggerDispenseAnimation();
    }

    // Confirmación de producto entregado por sensor IR
    if (line.includes('ITEM_DROPPED') || line.includes('ENTREGADO')) {
      this.onProductDropped(true);
    } else if (line.includes('ERROR_DROP') || line.includes('ERROR_DISPENSA')) {
      this.onProductDropped(false);
    }
  }

  // --- LÓGICA DE NAVEGACIÓN Y ESTADOS ---

  // Mover cursor arriba (ARRIBA)
  selectPreviousProduct(sendUart = true) {
    this.playClickSound();
    const total = this.catalogo().length;
    this.opcionSeleccionada.update(idx => (idx - 1 + total) % total);
    if (sendUart) {
      this.sendSerial('ARRIBA');
    }
    const prod = this.productoActual();
    this.ghostSpeech.set(`Seleccionaste: ${prod.nombre} (${prod.precio}). Presiona OK para pagar.`);
  }

  // Mover cursor abajo (ABAJO)
  selectNextProduct(sendUart = true) {
    this.playClickSound();
    const total = this.catalogo().length;
    this.opcionSeleccionada.update(idx => (idx + 1) % total);
    if (sendUart) {
      this.sendSerial('ABAJO');
    }
    const prod = this.productoActual();
    this.ghostSpeech.set(`Seleccionaste: ${prod.nombre} (${prod.precio}). Presiona OK para pagar.`);
  }

  // Elegir producto directamente por clic en la web
  selectDirectProduct(index: number) {
    this.playClickSound();
    this.opcionSeleccionada.set(index);
    const prod = this.productoActual();
    this.ghostSpeech.set(`Seleccionaste: ${prod.nombre} (${prod.precio}). Presiona OK para pagar.`);
    // Enviar comandos necesarios al ESP32 para emparejar selección
    this.sendSerial(`SELECT_${index}`);
  }

  // Botón OK / Enter (Entrar a pago o cancelar)
  pressOkButton(sendUart = true) {
    this.playClickSound();
    if (sendUart) {
      this.sendSerial('OK');
    }

    if (this.estadoActual() === 'MENU_SELECCION') {
      // Pasar a estado de pago
      this.estadoActual.set('ESPERANDO_PAGO');
      this.ghostEmotion.set('thinking');
      const prod = this.productoActual();
      this.ghostSpeech.set(`Canal: ${prod.nombre}. Total: ${prod.precio}. ¡Acerca tu tarjeta o pulsera NFC!`);
    } else if (this.estadoActual() === 'ESPERANDO_PAGO') {
      // Cancelar pago y regresar al menú
      this.cancelPayment(sendUart);
    }
  }

  // Cancelar proceso de pago
  cancelPayment(sendUart = true) {
    this.playClickSound();
    if (sendUart) {
      this.sendSerial('CANCEL');
    }
    this.estadoActual.set('MENU_SELECCION');
    this.ghostEmotion.set('idle');
    this.ghostSpeech.set('Operación cancelada. Elige otro producto cuando gustes.');
  }

  // Procesar pago NFC (físico o simulado)
  processCardTap(uidHex = '4A:8F:12:9C') {
    if (this.estadoActual() !== 'ESPERANDO_PAGO') {
      // Si no estaba en pago, iniciamos el pago del producto seleccionado
      this.pressOkButton(true);
    }

    this.playSuccessSound();
    this.lastScannedUid.set(uidHex);
    this.estadoActual.set('PAGO_EXITOSO');
    this.ghostEmotion.set('happy');
    const prod = this.productoActual();
    this.ghostSpeech.set(`¡NFC Detectado [${uidHex}]! Pago de ${prod.precio} aprobado. ¡Despachando ${prod.nombre}!`);

    // Notificar al ESP32 por USB para que pinte la pantalla de éxito
    this.sendSerial('HAPPY');

    // Activar animación y giro de despacho
    this.triggerDispenseAnimation();

    // Reducir stock
    this.catalogo.update(list =>
      list.map((p, idx) => idx === this.opcionSeleccionada() ? { ...p, stock: Math.max(0, p.stock - 1) } : p)
    );

    // Retorno automático al menú tras 4.5 segundos (igual que en esp32_vending_i2c_nfc.ino)
    setTimeout(() => {
      this.resetToMenu();
    }, 4500);
  }

  // Animación del Servomotor DS04-NFC en la Pico
  private triggerDispenseAnimation() {
    this.servoRunning.set(true);
    const dutyVal = this.servoDirectionInverted() ? 3800 : 6000;
    this.servoDuty.set(dutyVal);
    this.sendSerial('DESPACHAR');

    // Simular que el sensor IR detecta la caída a los 2.5s (si no viene por UART)
    if (!this.serialConnected()) {
      setTimeout(() => {
        this.simulateIRDrop();
      }, 2500);
    }
  }

  // Simulación de Sensor Infrarrojo IR
  simulateIRDrop() {
    if (!this.servoRunning()) return;
    this.irTriggered.set(true);
    this.playTone(1500, 'triangle', 0.1, 0.2);
    setTimeout(() => {
      this.irTriggered.set(false);
      this.onProductDropped(true);
    }, 400);
  }

  simulateIRTimeout() {
    if (!this.servoRunning()) return;
    this.onProductDropped(false);
  }

  onProductDropped(success: boolean) {
    this.servoRunning.set(false);
    this.servoDuty.set(4915); // Detenido
    if (success) {
      this.ghostSpeech.set('¡Producto detectado por el sensor IR! ¡Gracias por tu compra! 🍬👻');
    } else {
      this.ghostEmotion.set('surprised');
      this.ghostSpeech.set('¡Alerta! El sensor IR no detectó la caída del producto. Revisar stock o espiral.');
      this.playErrorSound();
    }
  }

  // Control manual del servomotor
  manualServoStart() {
    this.servoRunning.set(true);
    const dutyVal = this.servoDirectionInverted() ? 3800 : 6000;
    this.servoDuty.set(dutyVal);
    this.ghostSpeech.set(`Giro manual activado (Duty: ${dutyVal})`);
    this.sendSerial('DESPACHAR');
    this.playClickSound();
  }

  manualServoStop() {
    this.servoRunning.set(false);
    this.servoDuty.set(4915);
    this.ghostSpeech.set('Servomotor detenido (Punto neutro 4915).');
    this.sendSerial('STOP');
    this.playClickSound();
  }

  toggleServoDirection() {
    this.servoDirectionInverted.update(v => !v);
    this.playClickSound();
    if (this.servoRunning()) {
      const dutyVal = this.servoDirectionInverted() ? 3800 : 6000;
      this.servoDuty.set(dutyVal);
    }
  }

  setGhostEmotion(emotion: GhostEmotion) {
    this.ghostEmotion.set(emotion);
    this.playClickSound();
    if (emotion === 'idle') this.ghostSpeech.set('En reposo en el menú de productos...');
    if (emotion === 'thinking') this.ghostSpeech.set('Esperando pago NFC o tarjeta...');
    if (emotion === 'happy') this.ghostSpeech.set('¡Pago confirmado! Despachando con éxito.');
    if (emotion === 'surprised') this.ghostSpeech.set('¡Alerta de caída o producto cancelado!');
  }

  private resetToMenu() {
    this.estadoActual.set('MENU_SELECCION');
    this.ghostEmotion.set('idle');
    this.ghostSpeech.set('¡Listo para la siguiente orden! Elige un producto.');
  }
}
