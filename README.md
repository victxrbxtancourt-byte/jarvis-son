# Jarvis-son

[![CI](https://github.com/victxrbxtancourt-byte/jarvis-son/actions/workflows/ci.yml/badge.svg)](https://github.com/victxrbxtancourt-byte/jarvis-son/actions/workflows/ci.yml)

Asistente de voz **local** para un móvil modesto, corriendo en Termux. Es un proyecto académico (curso de Inteligencia Artificial) para estudiar una cadena de IA de voz y aprender integración, medición y decisiones de ingeniería con recursos limitados.

```
MIC → STT → SLM → TTS      conversación por turnos, sin conexión
```

| Etapa | Componente |
|---|---|
| Captura | Termux:API + FFmpeg (WAV mono, 16 kHz) |
| STT | whisper.cpp, modelo base (español) |
| SLM | llama.cpp con Qwen3.5-0.8B Q4_0 (contexto de 1.024 tokens, respuesta de hasta 96) |
| TTS | voz del sistema Android vía Termux:API |
| Orquestador | C++17 (POSIX), máquina de estados, un proceso por etapa |

## Alcance de la V1
- Conversa por turnos: el usuario habla, el sistema transcribe, genera una respuesta breve y la dice en voz alta.
- Funciona sin conexión una vez descargados los modelos.
- Registra tiempo y memoria por etapa, y errores, para comparar dispositivos.
- Fuera de alcance: palabra de activación permanente, interrupción de la voz, acciones sobre el sistema y fluidez de asistente en la nube.

## Empezar
Guía paso a paso en [`docs/instalacion.md`](docs/instalacion.md). Resumen, con todo instalado:

```bash
cmake -B build && cmake --build build -j2
ctest --test-dir build --output-on-failure
./build/jarvis-son --check
./build/jarvis-son --show-text
```

## Documentación
| Documento | Contenido |
|---|---|
| [`docs/instalacion.md`](docs/instalacion.md) | instalación, ejecución, límites y solución de problemas |
| [`docs/interfaces.md`](docs/interfaces.md) | contrato entre etapas |
| [`docs/orquestador.md`](docs/orquestador.md) | diagrama de estados, uso y seguridad del orquestador |
| [`docs/reporte-v1.md`](docs/reporte-v1.md) | rendimiento, calidad y problemas de la V1 |
| `docs/etapa-N-*.md` | evidencia de cada etapa |
| [`models/manifest.json`](models/manifest.json) | origen, licencia y checksum de los modelos y del software de terceros |

## Cómo trabajamos
- Una tarea = un issue = una rama corta = un pull request.
- Sin push directo a `main`; el CI debe pasar (compilación, pruebas y formato).
- No se suben modelos, audios, tokens ni datos del dispositivo.

## Privacidad
No se guardan grabaciones ni conversaciones: el audio se borra al terminar la transcripción y el registro de métricas solo contiene tiempos y memoria.

## Licencias
Cada modelo y programa de terceros tiene su propia licencia, anotada en `models/manifest.json`. Revisa la licencia de origen antes de redistribuir un modelo.