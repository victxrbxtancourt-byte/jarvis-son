# Etapa 4: Modelo de lenguaje pequeño (SLM)

## Configuración
- Motor: llama.cpp (commit: ______), compilado en Termux con CMake; se usa el binario `llama-completion` en modo de un solo turno (`-no-cnv`).
- Modelo: `Qwen3.5-0.8B-Q4_0.gguf` (563.036.064 bytes), descargado de `ggml-org/Qwen3.5-0.8B-GGUF`; origen, licencia y SHA-256 en `models/manifest.json`. El archivo no se versiona en Git.
- Parámetros: contexto de 1.024 tokens, salida máxima de 96 tokens, 4 hilos, `--temp 0.7 --top-k 20 --top-p 0.8`, semilla fija `--seed 42`.
- Prompt: formato ChatML con un mensaje de sistema ("Eres Jarvis-son, un asistente de voz. Responde siempre en español latinoamericano, en una o dos frases cortas, sin listas ni formato.") y un bloque `<think>` vacío para evitar que el modelo razone en voz alta.
- Teléfono: ver `docs/inventario-telefono.md`. RAM disponible al iniciar: ____ MB.
- Evaluación: 10 preguntas cortas de `docs/slm-preguntas.txt`, ejecutadas con `scripts/eval-slm.sh`. Cada pregunta lanza el proceso desde cero (carga el modelo en cada turno), y se miden tiempo total y memoria pico con `time` (`%e` y `%M`).
- Criterio de "adecuada": respuesta en español, breve, relevante y sin restos de razonamiento ni formato. Es un criterio de **forma**; no exige que el contenido sea factualmente correcto. Lo marcó la misma persona que hizo la prueba.

## Resultados

| Respuestas adecuadas | Latencia media (s) | Latencia mín–máx (s) | RAM pico (MB) |
|---|---|---|---|
| **8/10** | 4,44 | 3,39–5,17 | 1.169–1.178 |

Detalle por pregunta en `slm-resultados.csv`.

- **Repetibilidad:** en la segunda prueba mostro los mismos resultados que la primera sin ningun cambio.


## Análisis

**Forma y flujo (bueno).** El modelo responde en español, con frases cortas y sin restos de razonamiento (`<think>`), y corre sin cerrarse por memoria en los 10 turnos. Las respuestas más largas (preguntas 5 y 9) llegan a unas 30 palabras, dentro del límite de 96 tokens.

**Respuestas no adecuadas (2):**
- Pregunta 3, "¿Cuánto es doce por ocho?": respondió "Cinco por ocho." Falla de cálculo.
- Pregunta 10, "Gracias, hasta luego.": respondió con un saludo ("Hola. ¿En qué puedo ayudarte hoy?"). No reconoció la despedida.

**Calidad de contenido (limitada).** Más allá del criterio de forma, una lectura cualitativa del autor muestra que varias respuestas marcadas como adecuadas contienen errores:
- Pregunta 2: incluye el pepino entre las "frutas tropicales" y usa "la mango".
- Pregunta 4: el chiste no tiene sentido.
- Pregunta 5: la definición de asistente de voz es incorrecta.

Solo las preguntas 1, 6 (con ese error ortográfico), 7 y 9 son correctas en contenido sin reparos importantes. Esto es esperable en un modelo de 0.8B parámetros cuantizado a 4 bits, y se reporta tal cual: el modelo sirve para demostrar el flujo de voz, no para dar información confiable.

**Latencia.** Cada turno tarda entre 3,4 y 5,2 s, e incluye la carga del modelo desde cero. Sumada a la latencia del STT con el modelo base (~2,9 s), el recorrido de voz hasta el texto de respuesta ronda los 7 s, sin contar la captura ni el TTS. Es una estimación que se confirmará en la integración (Etapa 7).

**Memoria.** El pico observado fue de unos 1.170 MB, aproximadamente el doble del tamaño del archivo (563 MB), lo que confirma la advertencia del plan de que el tamaño del GGUF no equivale a la RAM de inferencia. Es también mayor que el pico del STT con el modelo base (~282 MB), por lo que la ejecución secuencial de las etapas (primero STT, luego SLM) es imprescindible: no deben convivir en memoria. El margen es estrecho en un equipo de 3 GB con Android y otros servicios consumiendo RAM, así que la prueba debe hacerse con el resto de apps cerradas.

## Decisión
- Se mantiene **Qwen3.5-0.8B Q4_0** con contexto de 1.024 tokens y salida máxima de 96 tokens para la V1: cumple el flujo (respuesta breve en español, sin cierres por memoria en 10 turnos).
- Se mantiene la ejecución estrictamente secuencial STT → SLM → TTS.


## Limitaciones
- Solo 10 preguntas, un solo teléfono y condiciones controladas; los resultados no se extrapolan a otros equipos.
- El criterio de "adecuada" evalúa la forma, no la veracidad, y lo aplicó una sola persona.
- La latencia incluye la carga del modelo en cada turno; una versión con el modelo residente en memoria sería más rápida, pero consumiría RAM de forma permanente.
- La memoria pico es el máximo de memoria residente que reporta el sistema; puede incluir páginas del modelo mapeadas desde el archivo, por lo que describe lo observado y no necesariamente la memoria no recuperable.


