# Reporte de la V1: rendimiento, calidad y problemas

Reporte reproducible de Jarvis-son V1 en el equipo de pruebas. Los números son de este teléfono y estas condiciones; no se extrapolan a otros móviles de 3 GB.

## Resumen
- **10 de 10 turnos consecutivos completados** de voz a voz, sin cierres por memoria ni pérdida de estado. Las 10 respuestas se escucharon en voz alta.
- **Turno completo:** p50 de 22,84 s y p95 de 30,62 s. La voz empieza a sonar unos 15 s después de iniciar la grabación (p50 de 15,15 s).
- **Memoria pico:** 1.178 MB, en el modelo de lenguaje; las etapas se ejecutan en secuencia y nunca coexisten en memoria.
- **Calidad:** la transcripción fue fiel en los 10 turnos, pero el contenido de las respuestas del modelo es limitado: solo 4 de 10 son correctas o razonables.
- **Meta de STT no alcanzada:** el criterio de 18 de 20 frases utilizables en silencio no se cumplió (16 de 20 con el mejor modelo).

## Equipo y configuración

| Elemento | Valor |
|---|---|
| Teléfono | Motorola moto g35 5G |
| SoC | Unisoc T760 (`ums9620`), 8 núcleos; se usan 4 hilos |
| RAM | 3.383 MB en total; 1.094 MB disponibles y 1.430 MB de swap en uso al iniciar la prueba |
| Android | 15 |
| Termux | 0.53.0 (f-droid) |
| STT | whisper.cpp con `ggml-base.bin`, idioma `es` |
| SLM | llama.cpp con `Qwen3.5-0.8B-Q4_0.gguf` (cuantización Q4_0), contexto de 1.024 tokens, salida máxima de 96 |
| TTS | voz del sistema Android (femenina, en español) vía Termux:API |
| Audio | clips de 5 s, WAV mono, 16 kHz |
| Orquestador | `jarvis-son`, C++17, un proceso por etapa |

Versiones exactas (commits de whisper.cpp y llama.cpp), origen, licencia y SHA-256 de cada modelo: `models/manifest.json`.

## Cómo reproducirlo

```bash
cmake -B build && cmake --build build -j2
ctest --test-dir build --output-on-failure
./build/jarvis-son --turns 10 --show-text --log docs/turnos-v1.csv
./scripts/summarize-turns.sh docs/turnos-v1.csv
```

Antes de medir: las preguntas de `docs/slm-preguntas.txt` dichas en orden, una por turno. Instalación completa en `docs/instalacion.md`. Datos por turno en `docs/turnos-v1.csv` (solo tiempos y memoria; el CSV no contiene texto de la conversación).

## Resultados de los 10 turnos

### Tiempo por etapa (segundos)

| Etapa | Mín | Media | p50 | p95 | Máx |
|---|---|---|---|---|---|
| Captura | 9,01 | 9,10 | 9,05 | 9,27 | 9,27 |
| STT (base) | 2,04 | 2,25 | 2,21 | 2,51 | 2,51 |
| SLM (Qwen3.5-0.8B) | 3,07 | 4,04 | 3,94 | 5,22 | 5,22 |
| TTS (sistema) | 3,68 | 7,27 | 6,16 | 14,36 | 14,36 |
| **Turno completo** | 17,97 | 22,66 | **22,84** | **30,62** | 30,62 |
| Hasta que empieza la voz (captura + STT + SLM) | 14,29 | 15,39 | 15,15 | 16,94 | 16,94 |

Los percentiles son por rango más cercano. Con 10 turnos, **p95 coincide con el máximo**, así que describe el peor caso observado y no una estimación estadística. El tiempo hasta la voz no incluye el arranque del motor de TTS.

### Memoria pico por proceso (MB)

| Etapa | Rango observado |
|---|---|
| Captura | 29–35 |
| STT (base) | 282–284 |
| SLM (Qwen3.5-0.8B) | 1.175–1.178 |
| TTS (`tts.sh`) | 5–6 |


### Detalle de cada turno

| # | Lo que se dijo (transcrito) | Respuesta del modelo | Se oyó | Contenido (lectura del autor) | Total (s) |
|---|---|---|---|---|---|
| 1 | ¿Cuál es la capital de Francia? | "Paris." | Sí | Correcto | 19,59 |
| 2 | Dime tres frutas tropicales. | "…son la mango, el pepino y la naranja." | Sí | Incorrecto (el pepino no es una fruta tropical) | 23,36 |
| 3 | cuanto es 12 por 8 | "8." | Sí | Incorrecto (son 96) | 17,97 |
| 4 | Cuéntame un chiste corto. | "El gato de la esquina no sabe que el hombre de la esquina tiene su propio gato." | Sí | No es un chiste; sin sentido | 23,33 |
| 5 | ¿Qué es un asistente de voz? | "Un asistente de voz es una herramienta que permite a una persona hablar sobre una temática específica…" | Sí | Incorrecto (definición equivocada) | 23,11 |
| 6 | ¿Cuántos días tiene febrero? | "Febriero tiene 28 días." | Sí | Correcto, con un error de ortografía | 20,06 |
| 7 | Dame un consejo para estudiar mejor. | "La mejor manera de estudiar es hacer las tareas primero y luego repasar…" | Sí | Razonable | 25,65 |
| 8 | como se prepara un café con leche | "Se prepara con leche fresca, se mezcla con granola y hierbas frescas." | Sí | Incorrecto (receta incoherente) | 22,84 |
| 9 | que es la inteligencia artificial. | "La inteligencia artificial es un sistema que utiliza algoritmos para aprender y procesar grandes cantidades de datos…" | Sí | Razonable | 30,62 |
| 10 | Gracias. Hasta luego. | "¡Hola! ¿En qué puedo ayudarte hoy?" | Sí | Inadecuado (saluda en vez de despedirse) | 20,08 |

La columna "Contenido" es una lectura cualitativa del autor, y no un criterio de la rúbrica del plan: **4 de 10** respuestas son correctas o razonables (turnos 1, 6, 7 y 9). Todas las transcripciones fueron fieles a lo dicho (10 de 10), aunque esta prueba de 10 turnos no sustituye la evaluación formal del STT (sección siguiente).

## Criterios de aceptación

| Criterio del plan | Resultado |
|---|---|
| 10 turnos consecutivos completos, sin OOM ni pérdida de estado | **Cumplido.** 10 de 10 turnos correctos, sin errores ni cierres; cada turno volvió a Listo y el siguiente arrancó con normalidad. |
| STT, SLM y TTS responden sin red tras descargar los recursos | Verificado el TTS en modo avión con 10 frases (Etapa 5). Corrida de 10 turnos en modo avión y sin internet. |
| Cada respuesta se reproduce en voz, sin solapar TTS e inferencia | **Cumplido.** Las 10 respuestas se oyeron. Cada etapa es un proceso aparte que termina antes de que empiece la siguiente. |
| ≥ 18 de 20 frases silenciosas transcritas de forma utilizable (y resultados con ruido aparte) | **No cumplido.** Modelo base: 16/20 en silencio (17/20 si se corrige la marca de la frase 20, que quedó como no utilizable aunque la transcripción coincidía con lo dicho). Modelo tiny: 10/20 en silencio y 7/20 con ruido. Detalle en `docs/etapa-3-stt.md`. |
| El repositorio compila en CI y documenta instalación, licencias, checksums y límites | **Cumplido.** CI en verde (compilación, 3 pruebas y formato); `docs/instalacion.md` y `models/manifest.json`. |
| Release V1 con manifiesto y limitaciones conocidas | Release `v1.0.0` del repositorio, con las notas de esta versión. |

## Análisis

**Rendimiento.** El recorrido es estable: la captura (9,0–9,3 s), el STT (2,0–2,5 s) y la memoria varían muy poco entre turnos. Dos etapas explican casi toda la variación del turno completo.
- **SLM:** su tiempo sigue de cerca la longitud de la respuesta (correlación de 0,96 sobre estos 10 turnos), porque genera más tokens cuando responde más.
- **TTS:** es la etapa más variable (3,7 a 14,4 s). Un ajuste lineal sobre estos 10 turnos da unos 4,7 s fijos más unos 3,3 s por cada 100 caracteres (correlación de 0,75). Es una estimación aproximada con pocos datos, pero indica un arranque fijo del motor de voz de varios segundos, además del tiempo de habla.

**Captura.** Una ventana de 5 s tarda unos 9 s en total: el resto es espera fija, arranque y cierre de la grabación y conversión. Es la etapa con más margen de mejora sin tocar los modelos.

**Memoria.** El pico de Qwen (1.178 MB) fue algo mayor que la RAM disponible al iniciar (1.094 MB), y aun así no hubo ningún cierre. Es coherente con que el sistema reutilizó memoria de caché y swap (había 1.430 MB de swap en uso), pero no se midió. Con otras apps abiertas, ese margen podría no alcanzar; por eso conviene cerrar apps antes de usar el asistente.

**Calidad.** El flujo funciona de extremo a extremo y la transcripción fue fiel, pero el modelo de lenguaje de 0.8B con cuantización de 4 bits comete errores factuales y de coherencia frecuentes (6 de 10 respuestas). No entendió la despedida del turno 10 y respondió con un saludo, igual que en la evaluación de la Etapa 4. El asistente sirve como demostración del flujo y de la medición, no como fuente de información confiable.

**STT.** La evaluación formal de 20 frases quedó por debajo de la meta: 16 de 20 en silencio con el modelo base. En la prueba de 10 turnos las 10 transcripciones fueron fieles; es una muestra pequeña, con las mismas preguntas del conjunto de pruebas, así que no cambia el resultado de la evaluación formal.

## Problemas encontrados durante el desarrollo
- **Error del STT que se propaga:** una transcripción errónea produce una respuesta incoherente; el modelo de lenguaje no puede corregirla (visto en la prueba manual de la cadena, previa al orquestador).
- **Marca `[end of text]` en la salida del modelo:** el TTS podía leerla; el orquestador la elimina antes de hablar.
- **Archivos de cabecera mezclados al copiar a mano:** la compilación falló por contenido cambiado entre archivos; se resolvió restaurando los archivos desde un paquete y verificando sumas SHA-256.
- **Saltos de línea CRLF de Windows en scripts:** romperían `bash` en Termux; se previene con `.gitattributes` y con una revisión en el CI.
- **Correo privado en commits (error GH007):** se resolvió configurando el correo de Git.
- **Voz Piper/sherpa-onnx:** no se evaluó; se usó el TTS del sistema por tiempo y consumo de recursos (`docs/etapa-5-tts.md`).

## Limitaciones
- Un solo teléfono, un solo hablante y 10 turnos; con ese tamaño, p95 es el máximo observado.
- Los tiempos incluyen la carga de cada modelo en cada turno, porque cada etapa arranca su proceso desde cero.
- La grabación es de duración fija y no detecta el fin del habla.
- Conversación por turnos: sin palabra de activación ni interrupción de la voz.
- La calidad de las respuestas es limitada; la valoración del contenido es subjetiva y de una sola persona.

## Privacidad
- No se guardaron audios ni conversaciones: el WAV se borra al terminar el STT y el CSV de la prueba solo contiene tiempos y memoria.
