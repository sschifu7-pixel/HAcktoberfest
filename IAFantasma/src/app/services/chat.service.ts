import { Injectable } from '@angular/core';
import { HttpClient, HttpHeaders } from '@angular/common/http';
import { Observable, of, delay, map, catchError } from 'rxjs';

export interface ChatMessage {
  id: string;
  role: 'user' | 'assistant';
  content: string;
  timestamp: Date;
}

export type GhostState = 'idle' | 'thinking' | 'talking' | 'happy' | 'surprised';

// =============================================
// CONFIGURACIÓN DE API EXTERNA
// Cambia USE_API a true y configura tu API_KEY
// para conectar con un servicio de IA real.
// =============================================
const USE_API = false; // ← Cambia a true para usar API externa
const API_URL = 'https://generativelanguage.googleapis.com/v1beta/models/gemini-pro:generateContent';
const API_KEY = 'TU_API_KEY_AQUI'; // ← Pon tu API key aquí

@Injectable({
  providedIn: 'root'
})
export class ChatService {

  private conversationHistory: ChatMessage[] = [];

  // Respuestas locales agrupadas por categorías
  private localResponses: Record<string, string[]> = {
    saludo: [
      '¡Buu! 👻 ¡Hola! Soy IAFantasma, tu asistente espectral. ¿En qué puedo ayudarte hoy?',
      '¡Aparecí justo a tiempo! Soy IAFantasma. ¿Qué necesitas saber?',
      '¡Guuu! ¡Me alegra que hayas llegado! ¿Cómo puedo asistirte?',
    ],
    despedida: [
      '¡Hasta pronto! Recuerda que siempre estaré en las sombras si me necesitas. 👻',
      '¡Adiós! Volveré a aparecer cuando me llames. ¡Cuídate!',
      'Me desvanezco por ahora... ¡pero siempre regreso! 🌙',
    ],
    nombre: [
      'Me llaman IAFantasma 👻 — ¡un espíritu digital aquí para ayudarte!',
      'Soy IAFantasma, el asistente más cool del mundo sobrenatural. 😎',
    ],
    ayuda: [
      'Puedo ayudarte con preguntas, conversación, consejos y mucho más. ¡Solo pregunta!',
      'Estoy aquí para charlar, informarte y hacerte compañía. ¿Por dónde empezamos?',
    ],
    gracias: [
      '¡De nada! Es un placer ser tu fantasma de confianza. 💫',
      '¡Para eso estoy! No dudes en volver cuando necesites algo. 👻',
      '¡Con mucho gusto! Los fantasmas somos más amigables de lo que parecemos. 😄',
    ],
    chiste: [
      '¿Por qué los fantasmas nunca mienten? ¡Porque son transparentes! 😄',
      '¿Qué hace un fantasma cuando se equivoca? ¡Se desaparece de vergüenza! 👻',
      '¿Cuál es el deporte favorito de los fantasmas? ¡El "boo"-ling! 🎳',
    ],
    default: [
      'Hmm, interesante pregunta... déjame pensarlo con mis poderes espectrales. 🔮',
      'Eso es algo que los fantasmas también nos preguntamos. ¿Quieres explorar el tema juntos?',
      'Mi sentido sobrenatural me dice que hay mucho que aprender sobre eso. 🌟',
      'Buena observación. Los espíritus tenemos mucho tiempo para reflexionar sobre esas cosas. 💭',
      'Como fantasma que soy, he visto muchas cosas... y sobre eso puedo decirte: ¡es fascinante!',
    ]
  };

  constructor(private http: HttpClient) {}

  sendMessage(userMessage: string): Observable<string> {
    if (USE_API) {
      return this.sendToAPI(userMessage);
    } else {
      return this.getLocalResponse(userMessage);
    }
  }

  private getLocalResponse(message: string): Observable<string> {
    const lower = message.toLowerCase();
    let responses: string[];

    if (/hola|buenos|buenas|hey|hi|saludos/.test(lower)) {
      responses = this.localResponses['saludo'];
    } else if (/adios|adiós|chao|hasta|bye|nos vemos/.test(lower)) {
      responses = this.localResponses['despedida'];
    } else if (/como te llamas|quien eres|quién eres|tu nombre|eres/.test(lower)) {
      responses = this.localResponses['nombre'];
    } else if (/ayuda|ayudar|puedes|qué haces|que haces|capacidades/.test(lower)) {
      responses = this.localResponses['ayuda'];
    } else if (/gracias|thank|agradec/.test(lower)) {
      responses = this.localResponses['gracias'];
    } else if (/chiste|broma|joke|risa|divertido/.test(lower)) {
      responses = this.localResponses['chiste'];
    } else {
      responses = this.localResponses['default'];
    }

    const response = responses[Math.floor(Math.random() * responses.length)];
    // Simulamos delay de "pensando" entre 800ms y 1800ms
    const thinkTime = 800 + Math.random() * 1000;
    return of(response).pipe(delay(thinkTime));
  }

  private sendToAPI(message: string): Observable<string> {
    // Formato para Gemini API
    const body = {
      contents: [{
        parts: [{
          text: `Eres IAFantasma, un asistente de IA amigable con personalidad de fantasma simpático. 
          Responde siempre en español, de forma breve y con algún toque de humor fantasmal ocasionalmente. 
          Mensaje del usuario: ${message}`
        }]
      }]
    };

    const headers = new HttpHeaders({ 'Content-Type': 'application/json' });

    return this.http.post<any>(`${API_URL}?key=${API_KEY}`, body, { headers }).pipe(
      map(res => res?.candidates?.[0]?.content?.parts?.[0]?.text ?? 'No pude obtener respuesta...'),
      catchError(() => of('Parece que perdí la señal espectral momentáneamente. ¡Intenta de nuevo! 👻'))
    );
  }

  getHistory(): ChatMessage[] {
    return this.conversationHistory;
  }

  addToHistory(message: ChatMessage): void {
    this.conversationHistory.push(message);
  }

  clearHistory(): void {
    this.conversationHistory = [];
  }

  generateId(): string {
    return Date.now().toString(36) + Math.random().toString(36).substr(2);
  }
}
