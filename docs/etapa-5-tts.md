# Etapa 5: Síntesis de voz (TTS)

## Configuración
- Motor: TTS integrado de Android, usado mediante Termux:API con `termux-tts-speak` (idioma `es`, región `MX`). El nombre del motor se obtiene con `termux-tts-engines`.
- Voz: femenina, en español (acento neutro).
- Punto único de salida de voz: `scripts/tts.sh`. Limpia el texto (quita caracteres de control), lo limita a 400 caracteres y lo entrega al motor por entrada estándar, sin armar comandos de shell con el texto.
- Evaluación: 10 frases de `docs/tts-frases.txt`, reproducidas con `scripts/eval-tts.sh`. Se registra la duración total de cada reproducción y una puntuación de 1 a 5 en pronunciación y naturalidad, asignada por la misma persona que hizo la prueba.
- Condiciones: **modo avión activado y sin conexión a internet** durante toda la evaluación.
- Teléfono: ver `docs/inventario-telefono.md`.

## Resultados

| Frases | Duración media (s) | Duración mín–máx (s) | Pronunciación (1–5) | Naturalidad (1–5) | Funciona sin red |
|---|---|---|---|---|---|
| 10 | 6,01 | 4,96–7,14 | **5,0** (9 de 10 registradas) | **4,0** | Sí |

Detalle por frase en `tts-resultados.csv`.

- **Frase 3** ("Febrero tiene veintiocho días."): la puntuación de pronunciación quedó en blanco en el registro por un error al teclearla. Según quien hizo la prueba, se pronunció correctamente. Por eso el promedio de pronunciación se calcula con 9 frases.
- Todas las frases se dijeron correctamente según la persona evaluadora, incluidas las de mayor dificultad: números ("noventa y seis"), tildes y la "ñ" ("Mañana"), y palabras largas ("murciélago", "maracuyá").
- La naturalidad fue 4 en las diez frases: la voz es clara y comprensible, aunque se percibe como sintética.

## Análisis

**Funcionamiento sin conexión.** Con el modo avión activado, la voz del sistema habló en español en las 10 frases. El plan advertía que el TTS de Android podía no funcionar sin red en todos los móviles; en este equipo sí funciona.

**Calidad.** Pronunciación excelente en un conjunto de frases con números, tildes y la "ñ". La naturalidad es buena pero no se evaluó frente a otras voces, por lo que no hay comparación.

**Duración.** Cada frase tardó entre 5 y 7 s. La medición va desde que se llama a `scripts/tts.sh` hasta que termina la voz, así que incluye el arranque del motor y el tiempo de habla, y no se pueden separar. No equivale al tiempo hasta oír la primera palabra, que no se midió.

**Recorrido de voz (estimación).** Con el STT base (~2,9 s) y el SLM (~4,4 s), el asistente empezaría a hablar unos 7 s después de terminar la captura, más el arranque del motor. Se confirmará en la integración (Etapa 7).

## Decisión
- Se usa el **TTS integrado del sistema** como motor de voz de la V1, a través de `scripts/tts.sh`.
- Motivos:
  - Tiempo disponible para el proyecto.
  - Consumo esperado de recursos: el motor alternativo de sherpa-onnx se distribuye como una app de Android de unas decenas de MB que correría en un proceso adicional, junto a Whisper y Qwen. Es una razón de planificación; **no se midió**.
  - Riesgo técnico: la búsqueda en los repositorios de Termux no encontró un paquete de Piper, y sherpa-onnx requiere instalar una aplicación externa o compilarlo. Sí existe `onnxruntime` (1.32.0) para aarch64 en Termux, por lo que la compilación podría ser viable.
- **No se evaluó Piper ni sherpa-onnx** con una voz en español. Queda como mejora futura; el cambio de motor se haría con la variable `TTS_ENGINE` de `scripts/tts.sh`.
- Como el motor es del sistema y no un modelo del proyecto, no se registra en `models/manifest.json`.

## Limitaciones
- La voz depende del motor y de los datos de voz instalados en este teléfono; en otro móvil puede sonar distinto o no tener voz en español sin conexión.
- Una sola persona evaluó, y la naturalidad fue 4 en todas las frases, lo que sugiere una escala poco discriminante.
- La memoria del motor no se midió: corre en otro proceso de Android y no se ve con `time`.
- No se midió el tiempo hasta la primera palabra.
- Solo 10 frases, un solo teléfono y una sola voz.
- No se probó una voz Piper ni se verificó la licencia de ninguna voz, porque se usa la del sistema.

