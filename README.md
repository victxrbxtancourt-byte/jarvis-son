# jarvis-son

Asistente de voz local para móvil (Termux): MIC → STT → SLM → TTS.
Proyecto académico, versión V1: conversación por turnos, sin conexión.

## Estado
Etapa 0 en curso.

## Equipo
- A: Termux, audio, Whisper/STT, medición del teléfono
- B: llama.cpp/Qwen, orquestador C++, TTS, documentación

## Cómo trabajamos
- Una tarea = un issue, una rama corta, un pull request.
- `main` solo recibe cambios revisados por otra persona.
- No se suben modelos, audios, tokens ni datos del dispositivo.

## Alcance V1
Ver `docs/`.