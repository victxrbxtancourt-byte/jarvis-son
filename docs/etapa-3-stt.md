# Etapa 3: Reconocimiento de voz (STT)

## Configuración
- whisper.cpp, compilado en Termux con CMake (`cmake -B build && cmake --build build -j2 --config Release`).
- Modelos evaluados: `ggml-tiny.bin` y `ggml-base.bin` (ambos multilingües), idioma fijado con `-l es`, 4 hilos de 8 disponibles, sin GPU (CPU con NEON y DOTPROD).
- Teléfono: ver `docs/inventario-telefono.md`. RAM disponible al iniciar: 1014 MB.
- Evaluación: 20 frases de `docs/stt-frases.txt`, grabadas con `scripts/capture.sh` (5 s, WAV mono 16 kHz) y transcritas con `scripts/eval-stt.sh`. Tiempo y memoria pico medidos con `time` (`%e` y `%M`) en cada frase.
- Criterio de "utilizable": el sentido de la frase se entiende y bastaría para que el modelo de lenguaje responda bien; no se exige coincidencia palabra por palabra. Lo marcó la misma persona que hizo la prueba.

## Resultados

### Primera ronda: modelo tiny

| Escenario | Utilizables | Latencia media (s) | Latencia mín–máx (s) | RAM pico (MB) |
|---|---|---|---|---|
| Silencio | **10/20** | 1,17 | 0,99–1,40 | 173–175 |
| Ruido moderado | **7/20** | 1,12 | 1,02–1,49 | 174–175 |

Detalle por frase en `stt-resultados-silencio.csv` y `stt-resultados-ruido.csv`.

Prueba de humo previa con `samples/jfk.wav` (inglés, 11 s): transcripción correcta en 1,62 s totales, incluida la carga del modelo.

Con tiny, la velocidad y la memoria fueron buenas, pero la calidad quedó muy por debajo de la meta del plan (18/20 en silencio): fallaron las frases 2, 3, 4, 8, 12, 13, 14, 16, 17 y 19. Con ruido bajó a 7/20 y los errores fueron más graves. Además, tres frases que fallaron en silencio (3, 4 y 12) salieron bien con ruido, lo que indica que la variación entre ejecuciones (voz, distancia al micrófono, momento en que se empieza a hablar dentro de la ventana de 5 s) pesa tanto como el ruido.

### Segunda ronda: modelo base

Por los bajos resultados de tiny, y como el plan lo permite cuando tiny queda corto, se evaluó el modelo **base** en silencio, con el mismo script, las mismas 20 frases y las mismas condiciones. Detalle por frase en `stt-resultados-base-silencio.csv`.

| Escenario | Utilizables | Latencia media (s) | Latencia mín–máx (s) | RAM pico (MB) |
|---|---|---|---|---|
| Silencio (base) | **16/20** | 2,86 | 2,41–3,86 | 281–283 |

### Comparación en silencio

| Modelo | Utilizables | Latencia media (s) | RAM pico (MB) |
|---|---|---|---|
| tiny | 10/20 | 1,17 | ~175 |
| base | **16/20** | 2,86 | ~282 |

Cambio de base respecto a tiny: **+6 frases utilizables**, a cambio de ~2,4 veces más latencia (+1,7 s por turno) y ~107 MB más de memoria pico.

## Análisis

**Calidad.** Base corrigió 7 frases que tiny fallaba (2, 3, 8, 12, 13, 14 y 16), entre ellas preguntas completas como "¿Qué hora es en Colombia?" y "¿Cuántos días tiene el mes de febrero?", y eliminó la transcripción truncada de la frase 12. Aun así quedó en 16/20, por debajo de la meta de 18/20.

**Frases que siguen fallando con base (4, 17, 19 y 20):**
- Frase 4, números: "¿Cuánto es doce por ocho?" → "cuánto es dos y por otro". Los números pronunciados en voz alta siguen siendo una debilidad.
- Frase 17: "Dame un consejo para estudiar mejor" → "...para subir mejor" (cambia el sentido).
- Frase 19: "Háblame del clima en un día lluvioso" → "hablame clima en india y curioso" (frase más larga con palabras poco frecuentes).
- Frase 20: "Gracias, hasta luego" quedó como rechazada pero feu un error humano mio, en realidad si quedo bien.

**Rendimiento.** Base tarda unos 2,9 s por clip de 5 s y usa unos 282 MB de pico. Es más lento y pesado que tiny, pero sigue siendo asumible: el plan ejecuta las etapas de forma secuencial, así que el pico de STT no se suma al de Qwen ni al del TTS, y quedaba alrededor de 1 GB de RAM disponible en el equipo.


**¿Se cumplió la meta de 18/20 en silencio?** No, con ninguno de los dos modelos: tiny 10/20 y base 16/20.

## Decisión
- Se adopta **base** como modelo de STT para la V1: es el que mejor calidad logra (16/20 frente a 10/20) y su costo (~2,9 s y ~282 MB) es asumible en este equipo.
- Se deja explícito que **la meta del plan no se alcanzó**. El resultado se reporta tal cual y no se extrapola a otros teléfonos.
- No se evaluó base con ruido ni el modelo `small`; este último se descarta por ahora por su mayor consumo de memoria frente a los ~1 GB disponibles.
- Mejoras posibles sin cambiar de modelo, pendientes de probar: acercar el micrófono, dictar con pausa corta antes de hablar, afinar la ventana de grabación, probar 6 hilos y usar una instrucción inicial (`--prompt`) con vocabulario del asistente.

## Limitaciones
- Conjunto pequeño (20 frases) y un solo hablante; los porcentajes tienen mucha varianza.
- La latencia incluye la carga del modelo en cada ejecución.
- El criterio "utilizable" es subjetivo y lo aplicó una sola persona. Por ejemplo, "Enciende la luz" → "entiende la luz de la sala" se aceptó, aunque un comando podría malinterpretarse.
- Base se evaluó solo en silencio y una sola vez; tiny se evaluó en silencio y con ruido.
- El tipo y el nivel de ruido no se midieron con instrumento; "moderado" es descriptivo.
- Resultados válidos solo para este teléfono y estas condiciones.

## Privacidad
- No se guardaron audios; los clips van a un temporal que se borra siempre. Solo se conservan textos de frases neutras.
- Los modelos no se suben al repositorio; sus datos (origen, licencia, SHA-256) están en `models/manifest.json`.