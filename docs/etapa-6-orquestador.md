# Etapa 6: Orquestador en C++

Programa `jarvis-son`, que conecta captura, STT, SLM y TTS en un turno de voz a voz. Diseño, opciones de uso, reglas de seguridad y diagrama de estados en [`orquestador.md`](orquestador.md); contrato de cada etapa en [`interfaces.md`](interfaces.md).

## Configuración de la prueba
- Teléfono: ver `docs/inventario-telefono.md`.
- Compilador: Clang 21.1.8 en Termux, C++17 y solo funciones POSIX, sin dependencias externas.
- STT: whisper.cpp con el modelo base. SLM: llama.cpp con Qwen3.5-0.8B Q4_0 (contexto de 1.024 tokens y salida máxima de 96). TTS: voz del sistema vía Termux:API.
- Ejecución: `./build/jarvis-son --show-text`, desde la raíz del repositorio, un turno por ejecución.

## Compilación reproducible

```bash
cmake -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

| Prueba | Qué comprueba | Resultado |
|---|---|---|
| `texto` | limpieza de transcripción y respuesta, marcadores de Whisper, tokens especiales, UTF-8 y prompt | Passed |
| `procesos` | salida, stdin, argumentos con metacaracteres, códigos de salida, señal 9, tiempo agotado, interrupción | Passed |
| `turno_con_stubs` | turno completo con programas simulados: éxito, transcripción vacía, fallo del STT, muerte del SLM, texto malicioso, limpieza de temporales, CSV sin texto, configuración inválida | Passed |

Resultado: **3 de 3 pruebas correctas** (2,25 s en total) en el teléfono. `./build/jarvis-son --check` valida las rutas y parámetros antes de ejecutar: `Configuración correcta.`

## Turnos de voz a voz

Tres turnos ejecutados con el teléfono de pruebas, con preguntas neutras. Cada turno recorrió los estados Grabando → Transcribiendo → Generando → Hablando → Listo sin errores.

| Turno | Transcripción (STT) | Respuesta (SLM) | Captura | STT | SLM | TTS | Total |
|---|---|---|---|---|---|---|---|
| 1 | "Hola, ¿cómo están?" | "Hola, cómo estás?" | 10,58 s | 2,36 s | 4,43 s | 6,05 s | 23,42 s |
| 2 | "cuál es la capital de Francia" | "Paris" | 10,70 s | 2,56 s | 3,96 s | 5,23 s | 22,46 s |
| 3 | "cual es la capita de Colombia" | "Colombia tiene una capital de Bogotá." | 9,35 s | 2,36 s | 3,64 s | 6,54 s | 21,90 s |

| Etapa | Tiempo medio | Mín–máx | Memoria pico del proceso |
|---|---|---|---|
| Captura | 10,21 s | 9,35–10,70 s | 30–35 MB |
| STT (base) | 2,43 s | 2,36–2,56 s | 283 MB |
| SLM (Qwen3.5-0.8B) | 4,01 s | 3,64–4,43 s | 1.178–1.179 MB |
| TTS (sistema) | 5,94 s | 5,23–6,54 s | 6 MB (solo `tts.sh`) |
| **Turno completo** | **22,59 s** | 21,90–23,42 s | — |

los tres turnos corrieron correctamente y la respuesta se reprodujo en voz alta. 

## Análisis

**El flujo funciona de extremo a extremo.** Las cuatro etapas se ejecutan en orden, cada una termina antes de que empiece la siguiente, y el orquestador mide el tiempo y la memoria de cada proceso. Esos datos coinciden con las evaluaciones por separado (STT base ~2,9 s y ~282 MB; SLM ~4,4 s y ~1.170 MB), lo que valida la medición.

**Memoria.** El pico es el del SLM, de unos 1.178 MB. Como las etapas son secuenciales, ese es el máximo del recorrido: el STT (283 MB) ya terminó cuando arranca Qwen, y los dos nunca coexisten en memoria. La memoria del orquestador mismo y la del motor de voz de Android (otro proceso) no se midieron.

**Latencia.** Un turno tarda unos 22,6 s en total, y la voz empieza a sonar unos 16,7 s después de iniciar la grabación. Los mayores consumos de tiempo no son los modelos:
- **Captura (~10 s para un clip de 5 s):** la ventana es fija y hay unos 5 s de espera y de arranque y cierre de la grabación y la conversión.
- **TTS (~6 s):** incluso la respuesta de una sola palabra ("Paris") tardó 5,23 s, lo que sugiere un arranque fijo del motor de voz de varios segundos, además del tiempo de habla.
- STT y SLM suman unos 6,4 s.


**Calidad.** El flujo es utilizable, pero la calidad la limitan los modelos pequeños:
- La transcripción puede perder puntuación y tildes ("cual es la capita de Colombia"). Aun con ese error, el SLM respondió bien.
- Las respuestas son muy breves o simplemente repiten la pregunta ("Paris"; "Hola, cómo estás?" ante un saludo), lo esperable en un modelo de 0.8B parámetros con instrucción de responder en una o dos frases.


## Seguridad y limpieza (verificadas con pruebas automáticas)
- Los procesos se lanzan con `execvp` y argumentos fijos, sin shell. El texto transcrito viaja como un único argumento del SLM, y el del TTS por entrada estándar. Una prueba con texto con metacaracteres de shell confirma que no se interpreta.
- Se distinguen el fallo normal, el tiempo agotado, la interrupción y la muerte por señal 9 (posible falta de memoria).
- Un turno con transcripción vacía no llama al SLM.
- Los temporales (directorio privado y WAV) se borran siempre, con éxito o con error.
- El registro CSV (`--log`) solo contiene tiempos y memoria, nunca el texto de la conversación.

## Limitaciones
- Las rutas de error (transcripción vacía, fallo de una etapa, muerte por señal, interrupción) se verificaron con programas simulados en `ctest`. No se probaron a mano en el teléfono en esta etapa.
- Los tiempos incluyen la carga del modelo en cada turno, porque cada etapa arranca su proceso desde cero.
- No se midió la memoria del orquestador ni la del motor de voz del sistema.
- Un solo turno a la vez; no hay interrupción de la voz mientras habla (fuera del alcance de la V1).
- Resultados válidos solo para este teléfono y estas condiciones.

## Privacidad
- No se guardan audios ni conversaciones: el WAV se borra al terminar el STT, y el texto solo vive en memoria durante el turno.
- Las preguntas de prueba son neutras.