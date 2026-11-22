# Guía de instalación (V1)

Cómo dejar Jarvis-son funcionando desde cero en un teléfono Android con Termux. Los comandos son los que se usaron en el equipo de pruebas (ver `docs/inventario-telefono.md`). Cada modelo se descarga de su fuente oficial y **no** se versiona en Git.

## Requisitos
- Android con arquitectura arm64 (`uname -m` debe decir `aarch64`) y unos 3 GB de RAM.
- Al menos 4 GB de almacenamiento libre (modelos, compilaciones y herramientas).
- Termux y Termux:API instalados **desde la misma fuente** oficial (F-Droid o GitHub Releases), no desde Play Store.
- Permiso de micrófono concedido a Termux:API.
- Voz de síntesis en español instalada en Android (Ajustes → Idiomas → Salida de texto a voz).
- Conexión a internet **solo para la instalación**; después funciona sin red.

## 1. Preparar Termux

```bash
pkg update && pkg upgrade -y
pkg install -y git cmake clang make ffmpeg termux-api time curl
termux-setup-storage
```

Comprueba el micrófono y la voz (debe oírse una frase en español):

```bash
termux-microphone-record -f ~/prueba.m4a -l 3 && sleep 4 && ls -lh ~/prueba.m4a && rm ~/prueba.m4a
termux-tts-speak -l es -n MX "Hola, esta es una prueba de voz."
```

## 2. Clonar el repositorio

```bash
cd ~
git clone https://github.com/victxrbxtancourt-byte/jarvis-son.git
cd jarvis-son
```

## 3. Reconocimiento de voz: whisper.cpp y modelo base

```bash
cd ~
git clone https://github.com/ggml-org/whisper.cpp.git
cd whisper.cpp
cmake -B build
cmake --build build -j2 --config Release
sh ./models/download-ggml-model.sh base
sha256sum models/ggml-base.bin
```

Compara el SHA-256 con el de `models/manifest.json` (entrada de `ggml-base.bin`). Si no coincide, la descarga está dañada: bórrala y repite. El manifiesto también indica el commit de whisper.cpp con el que se hicieron las pruebas.

## 4. Modelo de lenguaje: llama.cpp y Qwen3.5-0.8B Q4_0

```bash
cd ~
git clone --depth 1 https://github.com/ggml-org/llama.cpp.git
cd llama.cpp
cmake -B build -DCMAKE_BUILD_TYPE=Release -DLLAMA_CURL=OFF -DLLAMA_BUILD_TESTS=OFF
cmake --build build -j2 --target llama-completion
```

La compilación es larga. Si sale `Killed`, repite con `-j1`. Qwen3.5 necesita un llama.cpp reciente.

```bash
mkdir -p ~/models && cd ~/models
curl -L -C - -o Qwen3.5-0.8B-Q4_0.gguf \
  https://huggingface.co/ggml-org/Qwen3.5-0.8B-GGUF/resolve/main/Qwen3.5-0.8B-Q4_0.gguf
stat -c %s Qwen3.5-0.8B-Q4_0.gguf
sha256sum Qwen3.5-0.8B-Q4_0.gguf
```

El tamaño y el SHA-256 deben coincidir con `models/manifest.json`.

## 5. Compilar el orquestador

```bash
cd ~/jarvis-son
cmake -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

Debe terminar con `100% tests passed out of 3`. Las pruebas no necesitan micrófono ni modelos.

## 6. Verificar y ejecutar

Desde la raíz del repositorio:

```bash
./build/jarvis-son --check          # comprueba rutas y parámetros
./build/jarvis-son --show-text      # un turno de voz a voz
./build/jarvis-son --turns 10 --log docs/turnos-v1.csv
./scripts/summarize-turns.sh docs/turnos-v1.csv
```

Antes de medir, cierra las demás apps y conecta el teléfono al cargador. Habla cuando aparezca `[estado] Grabando`.

Si instalaste los programas en otras rutas, indícalas con variables de entorno: `JARVIS_WHISPER_BIN`, `JARVIS_WHISPER_MODEL`, `JARVIS_LLAMA_BIN`, `JARVIS_LLAMA_MODEL`, `JARVIS_CAPTURE`, `JARVIS_TTS` y `JARVIS_TMPDIR`. Detalles en `docs/orquestador.md`.

## 7. Funcionamiento sin conexión

Con todo descargado y compilado, activa el modo avión (y desactiva el Wi-Fi) y ejecuta `./build/jarvis-son --turns 10`. STT, SLM y TTS no necesitan red.

## Licencias y checksums

El origen, la licencia y el SHA-256 de cada modelo están en `models/manifest.json`. Antes de redistribuir un modelo o una voz, revisa su licencia en la página de origen. Este repositorio no incluye pesos de modelos.

## Límites conocidos

- Es una demo académica local: no garantiza atención continua ni respuesta inmediata.
- Conversación por turnos: sin palabra de activación permanente y sin interrupción de la voz mientras habla.
- La grabación dura un tiempo fijo (5 s por defecto) y no detecta el fin del habla.
- La calidad de la transcripción y de las respuestas es limitada por el tamaño de los modelos; un error del STT se propaga al resto del turno. Resultados medidos en `docs/etapa-3-stt.md`, `docs/etapa-4-slm.md` y `docs/reporte-v1.md`.
- Un turno completo tarda del orden de 20 s en el equipo de pruebas.
- La voz es la del sistema Android; depende de los datos de voz instalados en el teléfono.
- Las métricas corresponden a un teléfono concreto y no se extrapolan a otros móviles de 3 GB.
- La RAM es el recurso crítico: Qwen llega a más de 1,1 GB de memoria pico, así que conviene cerrar otras apps.

## Solución de problemas

| Síntoma | Qué revisar |
|---|---|
| `bad interpreter` o `\r` al ejecutar un script | El script quedó con saltos de línea CRLF. Se corrige con `sed -i 's/\r$//' scripts/*.sh` |
| `Killed` al compilar o al ejecutar el SLM | Falta de RAM: cierra apps, usa `-j1` al compilar |
| El micrófono graba vacío | Permiso de Termux:API y que ambas apps vengan de la misma fuente |
| `termux-tts-speak` habla en inglés o no habla | Datos de voz en español en los ajustes de Android |
| `unknown model architecture` | llama.cpp demasiado antiguo: `git pull` y recompilar |
| `jarvis-son --check` reporta rutas | Ver las variables de entorno de la sección 6 |
