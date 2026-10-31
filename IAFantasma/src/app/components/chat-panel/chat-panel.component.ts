import {
  Component, ViewChild, ElementRef, AfterViewChecked,
  ChangeDetectorRef, ChangeDetectionStrategy
} from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { ChatService, ChatMessage, GhostState } from '../../services/chat.service';
import { GhostAvatarComponent } from '../ghost-avatar/ghost-avatar.component';

@Component({
  selector: 'app-chat-panel',
  standalone: true,
  imports: [CommonModule, FormsModule, GhostAvatarComponent],
  templateUrl: './chat-panel.component.html',
  styleUrls: ['./chat-panel.component.css'],
  changeDetection: ChangeDetectionStrategy.Default
})
export class ChatPanelComponent implements AfterViewChecked {
  @ViewChild('messagesContainer') messagesContainer!: ElementRef;
  @ViewChild('inputEl') inputEl!: ElementRef;

  messages: ChatMessage[] = [];
  inputText = '';
  isLoading = false;
  ghostState: GhostState = 'idle';
  private shouldScroll = false;

  // Sugerencias rápidas
  quickSuggestions = [
    '¡Hola, Fantasma! 👋',
    '¿Quién eres?',
    'Cuéntame un chiste',
    '¿Qué puedes hacer?',
  ];

  constructor(
    private chatService: ChatService,
    private cdr: ChangeDetectorRef
  ) {
    // Mensaje de bienvenida
    this.messages.push({
      id: this.chatService.generateId(),
      role: 'assistant',
      content: '¡Buuu! 👻 ¡Bienvenido! Soy IAFantasma, tu asistente espectral. Estoy aquí para charlar, responder preguntas y hacer tu visita memorable. ¡Pregúntame lo que quieras!',
      timestamp: new Date()
    });
  }

  ngAfterViewChecked(): void {
    if (this.shouldScroll) {
      this.scrollToBottom();
      this.shouldScroll = false;
    }
  }

  sendMessage(text?: string): void {
    const msg = (text ?? this.inputText).trim();
    if (!msg || this.isLoading) return;

    // Agregar mensaje del usuario
    const userMsg: ChatMessage = {
      id: this.chatService.generateId(),
      role: 'user',
      content: msg,
      timestamp: new Date()
    };
    this.messages.push(userMsg);
    this.inputText = '';
    this.isLoading = true;
    this.ghostState = 'thinking';
    this.shouldScroll = true;

    // Enviar al servicio
    this.chatService.sendMessage(msg).subscribe({
      next: (response) => {
        const assistantMsg: ChatMessage = {
          id: this.chatService.generateId(),
          role: 'assistant',
          content: response,
          timestamp: new Date()
        };
        this.messages.push(assistantMsg);
        this.isLoading = false;
        this.ghostState = this.detectEmotion(response);
        this.shouldScroll = true;

        // Volver a idle después de animar la respuesta
        setTimeout(() => {
          if (this.ghostState !== 'thinking') {
            this.ghostState = 'idle';
            this.cdr.markForCheck();
          }
        }, 3000);

        this.cdr.markForCheck();
      },
      error: () => {
        this.messages.push({
          id: this.chatService.generateId(),
          role: 'assistant',
          content: 'Perdí la conexión espectral momentáneamente... 👻 ¡Inténtalo de nuevo!',
          timestamp: new Date()
        });
        this.isLoading = false;
        this.ghostState = 'idle';
        this.shouldScroll = true;
        this.cdr.markForCheck();
      }
    });
  }

  onKeyDown(event: KeyboardEvent): void {
    if (event.key === 'Enter' && !event.shiftKey) {
      event.preventDefault();
      this.sendMessage();
    }
  }

  clearChat(): void {
    this.messages = [{
      id: this.chatService.generateId(),
      role: 'assistant',
      content: '¡El chat fue limpiado! Empecemos de nuevo. 👻 ¿En qué puedo ayudarte?',
      timestamp: new Date()
    }];
    this.chatService.clearHistory();
    this.ghostState = 'happy';
    setTimeout(() => { this.ghostState = 'idle'; this.cdr.markForCheck(); }, 2000);
  }

  trackById(_i: number, msg: ChatMessage): string {
    return msg.id;
  }

  formatTime(date: Date): string {
    return date.toLocaleTimeString('es-MX', { hour: '2-digit', minute: '2-digit' });
  }

  private detectEmotion(text: string): GhostState {
    const lower = text.toLowerCase();
    if (/\!|genial|excelente|😄|✨|🌟|fantástico|😎/.test(lower)) return 'happy';
    if (/\?|sorprendente|increíble|vaya|😲/.test(lower)) return 'surprised';
    return 'talking';
  }

  private scrollToBottom(): void {
    try {
      const el = this.messagesContainer.nativeElement;
      el.scrollTop = el.scrollHeight;
    } catch {}
  }
}
