# Interfaces entre etapas (Jarvis-son V1)

Contrato de cada etapa antes de integrarlas en el orquestador (Etapa 6). Cada etapa se lanza como un proceso independiente, con argumentos fijos, y termina antes de iniciar la siguiente.

## Flujo

```
Captura (WAV) → STT (texto) → SLM (texto) → TTS (voz)
```

## Contrato por etapa

| Etapa | Entrada | Comando | Salida | Fallo |
|---|---|---|---|---|
| Captura | duración (1 a 30 s) y ruta del WAV | `scripts/capture.sh <segundos> <ruta.wav>` | WAV mono, 16 kHz, PCM 16 bits | código ≠ 0 y mensaje `ERROR:` en stdout |
| STT | ruta del WAV | `whisper-cli -m <ggml-base.bin> -f <wav> -l es -t 4 -nt -np` | texto transcrito por stdout | código ≠ 0 o texto vacío |
| SLM | prompt en formato ChatML | `llama-completion -m <Qwen3.5-0.8B-Q4_0.gguf> -c 1024 -n 96 -t 4 --seed 42 --temp 0.7 --top-k 20 --top-p 0.8 -no-cnv --no-display-prompt -p <prompt>` (stdin desde `/dev/null`) | respuesta por stdout | código ≠ 0, muerte por señal (por ejemplo, 9 por falta de memoria) o texto vacío |
| TTS | texto por entrada estándar | `scripts/tts.sh` | voz por el altavoz, sin archivo | código ≠ 0 |

Rutas de referencia en el equipo de pruebas: `~/whisper.cpp/build/bin/whisper-cli`, `~/whisper.cpp/models/ggml-base.bin`, `~/llama.cpp/build/bin/llama-completion` y `~/models/Qwen3.5-0.8B-Q4_0.gguf`. Los modelos no se versionan; sus datos están en `models/manifest.json`.

## Formato del prompt del SLM

ChatML con un bloque `<think>` vacío para evitar el razonamiento visible:

```
<|im_start|>system
Eres Jarvis-son, un asistente de voz. Responde siempre en español latinoamericano, en una o dos frases cortas, sin listas ni formato.<|im_end|>
<|im_start|>user
{texto transcrito}<|im_end|>
<|im_start|>assistant
<think>

</think>

```

## Reglas de limpieza entre etapas

Observadas en la prueba manual de referencia:

- **STT → SLM:** la salida de `whisper-cli` empieza con salto de línea y espacio. Hay que recortar espacios y saltos de línea al inicio y al final. Los mensajes de progreso (`read_audio_data: ...`) salen por stderr y se descartan.
- **SLM → TTS:** la salida de `llama-completion` puede terminar con la marca `[end of text]`. Hay que eliminarla antes de hablar; si no, el TTS puede leerla en voz alta.
- **Texto vacío:** si la transcripción queda vacía tras recortar, no se llama al SLM: el turno termina en estado de error ("no se entendió"). Lo mismo si la respuesta del SLM queda vacía.
- **Seguridad:** el texto transcrito y el texto del modelo nunca se concatenan en una línea de comandos de shell. Se pasan como argumento separado (`execve`) o por entrada estándar.

## Limpieza y fallos

- Los archivos temporales (WAV del turno) se eliminan siempre al terminar el turno, con éxito o con error.
- El orquestador debe verificar el código de salida de cada proceso y distinguir el fallo normal de la muerte por señal (por ejemplo, `Killed` por falta de memoria).
- Un turno que falla vuelve al estado Listo sin dejar procesos ni temporales huérfanos.

## Prueba manual de referencia (un turno completo)

Un solo turno, ejecutado a mano con el teléfono cargando. Sirve de punto de partida para medir el orquestador; no son estadísticas (p50 y p95 se reportan en la Etapa 7).

| Etapa | Tiempo (s) | Notas |
|---|---|---|
| Captura | 9,2 y 10,8 (dos ejecuciones) | Ventana de 5 s; el resto es espera fija y arranque |
| STT (base) | 2,75 | |
| SLM (Qwen3.5-0.8B Q4_0) | 5,54 | Incluye la carga del modelo |
| TTS (sistema) | 8,51 | Incluye el habla completa |

Memoria al iniciar la prueba: 3.383 MB en total, 1.126 MB disponibles, con unos 1.533 MB de swap en uso.

Resultado de la prueba: la transcripción fue "Cuidad a capital de Francia." (la frase dicha era una pregunta sobre la capital de Francia), y el SLM respondió con un saludo en vez de contestar la pregunta. Un error del STT se propaga al resto del flujo; el SLM no puede corregirlo.

## Estados del orquestador

```
Listo → Grabando → Transcribiendo → Generando → Hablando → Listo
                      cualquier etapa con fallo → Error → Listo
```

Cada transición registra el tiempo de la etapa y su resultado.