import { Injectable, signal, inject } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { VendingService } from './vending.service';

export interface VoiceOrderIntent {
  rawTranscript: string;
  detectedProductIndex?: number;
  productName?: string;
  action: 'SELECT_PRODUCT' | 'PROCEED_PAYMENT' | 'CANCEL' | 'DISPENSE' | 'UNKNOWN';
  confidence: number;
}

@Injectable({
  providedIn: 'root'
})
export class VoiceWorkflowService {
  private http = inject(HttpClient);
  private vending = inject(VendingService);

  // --- CONFIGURACIÓN ELEVENLABS ---
  elevenLabsApiKey = signal<string>(
    localStorage.getItem('elevenlabs_api_key') || ''
  );
  elevenLabsVoiceId = signal<string>(
    localStorage.getItem('elevenlabs_voice_id') || '21m00Tcm4TlvDq8ikWAM' // Default: Rachel (Voz clara y espectral)
  );
  elevenLabsModel = signal<string>('eleven_multilingual_v2');
  isGeneratingAudio = signal<boolean>(false);
  isPlayingAudio = signal<boolean>(false);

  // --- CONFIGURACIÓN n8n WEBHOOK ---
  n8nWebhookUrl = signal<string>(
    localStorage.getItem('n8n_webhook_url') || 'https://mak0ia0.app.n8n.cloud/webhook/vending-order'
  );
  lastN8nStatus = signal<string>('Listo');
  isSendingToN8n = signal<boolean>(false);

  // --- ESTADO DE RECONOCIMIENTO DE VOZ (STT) ---
  isListening = signal<boolean>(false);
  speechSupported = signal<boolean>(false);
  transcript = signal<string>('');
  lastIntent = signal<string>('Esperando orden de voz...');
  showConfigModal = signal<boolean>(false);

  private recognition: any = null;
  private currentAudio: HTMLAudioElement | null = null;

  constructor() {
    this.initSpeechRecognition();
  }

  // Guardar configuración en localStorage
  saveConfig(apiKey: string, voiceId: string, n8nUrl: string) {
    this.elevenLabsApiKey.set(apiKey.trim());
    this.elevenLabsVoiceId.set(voiceId.trim() || '21m00Tcm4TlvDq8ikWAM');
    this.n8nWebhookUrl.set(n8nUrl.trim());

    localStorage.setItem('elevenlabs_api_key', apiKey.trim());
    localStorage.setItem('elevenlabs_voice_id', voiceId.trim());
    localStorage.setItem('n8n_webhook_url', n8nUrl.trim());

    this.showConfigModal.set(false);
    this.vending.addLog('sys', 'Configuración de ElevenLabs y n8n actualizada con éxito.');
  }

  // =========================================================================
  // 1. RECONOCIMIENTO DE VOZ (Web Speech API STT)
  // =========================================================================
  private initSpeechRecognition() {
    const SpeechRecognition = (window as any).SpeechRecognition || (window as any).webkitSpeechRecognition;
    if (SpeechRecognition) {
      this.speechSupported.set(true);
      this.recognition = new SpeechRecognition();
      this.recognition.lang = 'es-MX'; // Español latinoamericano
      this.recognition.continuous = false;
      this.recognition.interimResults = true;

      this.recognition.onstart = () => {
        this.isListening.set(true);
        this.transcript.set('Escuchando tu voz...');
        this.vending.addLog('sys', '🎙️ Micrófono activado: Escuchando comando de voz...');
      };

      this.recognition.onresult = (event: any) => {
        let interim = '';
        let final = '';
        for (let i = event.resultIndex; i < event.results.length; ++i) {
          if (event.results[i].isFinal) {
            final += event.results[i][0].transcript;
          } else {
            interim += event.results[i][0].transcript;
          }
        }
        const text = final || interim;
        this.transcript.set(text);

        if (final) {
          this.processVoiceCommand(final);
        }
      };

      this.recognition.onerror = (event: any) => {
        console.warn('Error en SpeechRecognition:', event.error);
        this.isListening.set(false);
        this.transcript.set(`Error de micrófono: ${event.error}`);
        this.vending.addLog('sys', `Error micrófono: ${event.error}`);
      };

      this.recognition.onend = () => {
        this.isListening.set(false);
      };
    } else {
      this.speechSupported.set(false);
      console.warn('Web Speech API no está soportada en este navegador.');
    }
  }

  toggleListening() {
    if (!this.speechSupported()) {
      alert('Tu navegador no soporta Web Speech API para reconocimiento de voz. Usa Google Chrome o Microsoft Edge.');
      return;
    }

    if (this.isListening()) {
      this.recognition?.stop();
      this.isListening.set(false);
    } else {
      try {
        this.recognition?.start();
      } catch (e) {
        console.error('Error al iniciar micrófono:', e);
        this.recognition?.stop();
        setTimeout(() => this.recognition?.start(), 200);
      }
    }
  }

  // =========================================================================
  // 2. PROCESAMIENTO DE LENGUAJE NATURAL E INTENCIÓN DE COMPRA
  // =========================================================================
  processVoiceCommand(phrase: string) {
    const text = phrase.toLowerCase().trim();
    this.vending.addLog('rx', `Voz Detectada: "${phrase}"`);

    let targetProductIndex = -1;
    let action: VoiceOrderIntent['action'] = 'UNKNOWN';
    let responseText = '';

    // A. Detectar intención de producto
    if (text.includes('soda') || text.includes('refresco') || text.includes('bebida') || text.includes('lata') || text.includes('uno') || text.includes('1') || text.includes('a1')) {
      targetProductIndex = 0; // Soda Web3
      action = 'SELECT_PRODUCT';
    } else if (text.includes('snack') || text.includes('chocolate') || text.includes('barra') || text.includes('cyber') || text.includes('dos') || text.includes('2') || text.includes('a2')) {
      targetProductIndex = 1; // Cyber Snack
      action = 'SELECT_PRODUCT';
    } else if (text.includes('energy') || text.includes('energía') || text.includes('energetica') || text.includes('crypto') || text.includes('tres') || text.includes('3') || text.includes('a3')) {
      targetProductIndex = 2; // Crypto Energy
      action = 'SELECT_PRODUCT';
    }

    // B. Detectar intención de control / pago / cancelación
    if (text.includes('pagar') || text.includes('comprar') || text.includes('confirmar') || text.includes('ok') || text.includes('tarjeta') || text.includes('nfc')) {
      action = 'PROCEED_PAYMENT';
    } else if (text.includes('cancelar') || text.includes('atras') || text.includes('atrás') || text.includes('no') || text.includes('volver') || text.includes('menú')) {
      action = 'CANCEL';
    } else if (text.includes('despachar') || text.includes('servir') || text.includes('entregar') || text.includes('motor')) {
      action = 'DISPENSE';
    }

    // C. Ejecutar la acción en la máquina
    if (action === 'SELECT_PRODUCT' && targetProductIndex >= 0) {
      const prod = this.vending.catalogo()[targetProductIndex];
      this.vending.selectDirectProduct(targetProductIndex);
      this.vending.pressOkButton(true); // Pasar a estado de pago
      this.lastIntent.set(`Orden por Voz: ${prod.nombre} (${prod.precio})`);

      responseText = `¡Excelente elección espectral! Has pedido ${prod.nombre}. El precio es ${prod.precio}. Por favor acerca tu tarjeta NFC para completar el pago.`;
      
      // Enviar webhook de flujo a n8n
      this.triggerN8nWorkflow({
        event: 'VOICE_ORDER_RECEIVED',
        phrase: phrase,
        product: prod,
        action: 'SELECT_AND_WAIT_NFC',
        status: 'ESPERANDO_PAGO'
      });

    } else if (action === 'PROCEED_PAYMENT') {
      const prod = this.vending.productoActual();
      this.vending.pressOkButton(true);
      responseText = `Listo, esperando tu tarjeta o pulsera NFC para pagar ${prod.nombre}.`;
      this.lastIntent.set(`Confirmación de pago para ${prod.nombre}`);

      this.triggerN8nWorkflow({
        event: 'PAYMENT_REQUESTED_BY_VOICE',
        phrase: phrase,
        product: prod,
        action: 'WAIT_NFC'
      });

    } else if (action === 'CANCEL') {
      this.vending.cancelPayment(true);
      responseText = `Operación cancelada. Puedes pedirme otro producto con tu voz cuando quieras.`;
      this.lastIntent.set('Orden cancelada por voz');

      this.triggerN8nWorkflow({
        event: 'ORDER_CANCELLED_BY_VOICE',
        phrase: phrase,
        action: 'CANCELLED'
      });

    } else if (action === 'DISPENSE') {
      this.vending.manualServoStart();
      responseText = `¡Comando de despacho activado! El motor B está girando en la Pico.`;
      this.lastIntent.set('Despacho activado por voz');

    } else {
      responseText = `Disculpa, escuché "${phrase}", pero no estoy seguro de qué producto deseas. Prueba diciendo "Quiero una Soda Web3", "Dame un Snack" o "Crypto Energy".`;
      this.lastIntent.set(`No reconocido: "${phrase}"`);
    }

    // D. Responder con la voz de ElevenLabs
    this.speakWithElevenLabs(responseText);
  }

  // =========================================================================
  // 3. GENERACIÓN DE VOZ ESPECTRAL CON ELEVENLABS (TTS)
  // =========================================================================
  async speakWithElevenLabs(text: string) {
    const apiKey = this.elevenLabsApiKey().trim();
    const voiceId = this.elevenLabsVoiceId().trim() || '21m00Tcm4TlvDq8ikWAM';

    this.vending.ghostSpeech.set(text);

    // Si no hay API key configurada, usar síntesis nativa de voz como fallback
    if (!apiKey) {
      this.fallbackNativeSpeech(text);
      return;
    }

    this.isGeneratingAudio.set(true);
    this.vending.addLog('tx', `ElevenLabs TTS -> Generando voz para: "${text.substring(0, 40)}..."`);

    try {
      const url = `https://api.elevenlabs.io/v1/text-to-speech/${voiceId}`;
      const payload = {
        text: text,
        model_id: this.elevenLabsModel(),
        voice_settings: {
          stability: 0.45,
          similarity_boost: 0.85,
          style: 0.35,
          use_speaker_boost: true
        }
      };

      const response = await fetch(url, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'xi-api-key': apiKey
        },
        body: JSON.stringify(payload)
      });

      if (!response.ok) {
        const errText = await response.text();
        throw new Error(`Error ElevenLabs ${response.status}: ${errText}`);
      }

      const audioBlob = await response.blob();
      const audioUrl = URL.createObjectURL(audioBlob);

      this.playAudioUrl(audioUrl);
      this.vending.addLog('rx', 'ElevenLabs TTS -> Audio generado y reproduciendo.');

    } catch (error: any) {
      console.warn('Fallo ElevenLabs API, usando fallback nativo:', error);
      this.vending.addLog('sys', `ElevenLabs aviso: ${error.message}. Usando voz local.`);
      this.fallbackNativeSpeech(text);
    } finally {
      this.isGeneratingAudio.set(false);
    }
  }

  private playAudioUrl(url: string) {
    if (this.currentAudio) {
      this.currentAudio.pause();
    }
    this.currentAudio = new Audio(url);
    this.isPlayingAudio.set(true);

    this.currentAudio.onplay = () => {
      this.isPlayingAudio.set(true);
    };

    this.currentAudio.onended = () => {
      this.isPlayingAudio.set(false);
      URL.revokeObjectURL(url);
    };

    this.currentAudio.onerror = () => {
      this.isPlayingAudio.set(false);
    };

    this.currentAudio.play().catch(e => {
      console.warn('Reproducción de audio bloqueada por el navegador:', e);
      this.isPlayingAudio.set(false);
    });
  }

  // Fallback nativo de síntesis de voz (Web Speech Synthesis) con tono fantasmal
  private fallbackNativeSpeech(text: string) {
    if (!('speechSynthesis' in window)) return;
    window.speechSynthesis.cancel();

    const utterance = new SpeechSynthesisUtterance(text);
    utterance.lang = 'es-ES';
    utterance.pitch = 0.85; // Un poco grave / espectral
    utterance.rate = 1.0;

    utterance.onstart = () => this.isPlayingAudio.set(true);
    utterance.onend = () => this.isPlayingAudio.set(false);
    utterance.onerror = () => this.isPlayingAudio.set(false);

    window.speechSynthesis.speak(utterance);
  }

  // =========================================================================
  // 4. FLUJO DE INFORMACIÓN CON n8n (WEBHOOK INTEGRATION)
  // =========================================================================
  async triggerN8nWorkflow(payloadData: any): Promise<boolean> {
    const webhookUrl = this.n8nWebhookUrl().trim();
    if (!webhookUrl) {
      this.lastN8nStatus.set('URL no configurada');
      return false;
    }

    this.isSendingToN8n.set(true);
    this.lastN8nStatus.set('Enviando a n8n...');

    const completePayload = {
      timestamp: new Date().toISOString(),
      machine_id: 'VENDING_WEB3_ESP32_01',
      device: {
        esp32_ip: this.vending.esp32Ip(),
        usb_connected: this.vending.serialConnected(),
        wifi_connected: this.vending.wifiConnected()
      },
      telemetry: {
        servo_duty: this.vending.servoDuty(),
        servo_running: this.vending.servoRunning(),
        ir_sensor_drop: this.vending.irTriggered(),
        nfc_tag_detected: this.vending.lastScannedUid()
      },
      ...payloadData
    };

    this.vending.addLog('tx', `n8n Webhook [${payloadData.event}] -> ${webhookUrl}`);

    try {
      // Usamos fetch con modo cors / no-cors para máxima tolerancia de n8n
      const response = await fetch(webhookUrl, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify(completePayload)
      });

      this.lastN8nStatus.set('Enviado OK (200)');
      this.vending.addLog('rx', `n8n Webhook: Flujo de información procesado correctamente.`);
      return true;

    } catch (err: any) {
      console.warn('Aviso al enviar a n8n:', err);
      this.lastN8nStatus.set('Error / Sin conexión n8n');
      this.vending.addLog('sys', `n8n Webhook aviso: No se pudo conectar a ${webhookUrl}. Asegúrate de que n8n esté corriendo.`);
      return false;
    } finally {
      this.isSendingToN8n.set(false);
    }
  }
}
