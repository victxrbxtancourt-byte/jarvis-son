#!/bin/sh
# Prueba de extremo a extremo del orquestador con programas simulados (stubs).
# No necesita micrófono, modelos ni teléfono: sirve en Termux y en CI (Linux).
# Uso: tests/run_stub_turn.sh <ruta al ejecutable jarvis-son>
set -u

BIN="$1"
HERE="$(cd "$(dirname "$0")" && pwd)"
chmod +x "$HERE"/stubs/*.sh

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
TMPD="$WORK/tmp"
mkdir "$TMPD"
: > "$WORK/modelo.bin"

export JARVIS_CAPTURE="$HERE/stubs/capture.sh"
export JARVIS_TTS="$HERE/stubs/tts.sh"
export JARVIS_WHISPER_BIN="$HERE/stubs/whisper.sh"
export JARVIS_WHISPER_MODEL="$WORK/modelo.bin"
export JARVIS_LLAMA_BIN="$HERE/stubs/llama.sh"
export JARVIS_LLAMA_MODEL="$WORK/modelo.bin"
export JARVIS_TMPDIR="$TMPD"
export JARVIS_STUB_DIR="$WORK"

fail() {
    echo "FALLO: $1"
    exit 1
}
tmp_limpio() { [ -z "$(ls -A "$TMPD")" ]; }

# 1) Dos turnos correctos ----------------------------------------------------
"$BIN" --turns 2 --log "$WORK/log.csv" 2> "$WORK/err.txt" || fail "turnos correctos terminaron con error"
[ "$(cat "$WORK/tts_in.txt")" = "La capital de Francia es París." ] || fail "texto entregado al TTS incorrecto"
grep -q "end of text" "$WORK/tts_in.txt" && fail "el TTS recibió la marca [end of text]"
grep -q "Cuál es la capital de Francia\." "$WORK/prompt.txt" || fail "el prompt no tiene la transcripción limpia"
grep -q "<think>" "$WORK/prompt.txt" || fail "el prompt no tiene el bloque think"
tmp_limpio || fail "quedaron temporales tras un turno correcto"
[ "$(wc -l < "$WORK/log.csv")" -eq 3 ] || fail "el registro debe tener encabezado y 2 filas"
grep -q "Francia" "$WORK/log.csv" && fail "el registro contiene texto de la conversación"
grep -q "^1;ok;-;" "$WORK/log.csv" || fail "fila de registro inesperada"
grep -q "\[estado\] Grabando" "$WORK/err.txt" || fail "falta el estado Grabando"
grep -q "\[estado\] Hablando" "$WORK/err.txt" || fail "falta el estado Hablando"

# 2) Transcripción vacía: no se llama al SLM -----------------------------------
rm -f "$WORK/llama_called"
JARVIS_STUB_WHISPER_EMPTY=1 "$BIN" 2> "$WORK/err.txt"
[ $? -eq 1 ] || fail "transcripción vacía debía terminar con código 1"
[ ! -e "$WORK/llama_called" ] || fail "se llamó al SLM con transcripción vacía"
grep -q "\[fallo en Transcribiendo\]" "$WORK/err.txt" || fail "mensaje de fallo inesperado (vacía)"
tmp_limpio || fail "quedaron temporales tras transcripción vacía"

# 3) Falla el STT (código ≠ 0) --------------------------------------------------
rm -f "$WORK/llama_called"
JARVIS_STUB_WHISPER_FAIL=1 "$BIN" 2> "$WORK/err.txt"
[ $? -eq 1 ] || fail "fallo del STT debía terminar con código 1"
[ ! -e "$WORK/llama_called" ] || fail "se llamó al SLM tras un fallo del STT"
grep -q "código de salida 4" "$WORK/err.txt" || fail "no se informó el código de salida del STT"
tmp_limpio || fail "quedaron temporales tras fallo del STT"

# 4) El SLM muere por señal 9 (falta de memoria) ---------------------------------
JARVIS_STUB_LLAMA_KILL=1 "$BIN" 2> "$WORK/err.txt"
[ $? -eq 1 ] || fail "muerte del SLM debía terminar con código 1"
grep -q "señal 9" "$WORK/err.txt" || fail "no se informó la señal 9"
tmp_limpio || fail "quedaron temporales tras la muerte del SLM"

# 5) Texto con metacaracteres de shell: nunca se interpreta ----------------------
rm -f "$WORK/CANARIO"
JARVIS_STUB_WHISPER_TEXT='uno; touch '"$WORK"'/CANARIO && echo `id` | cat "x"' "$BIN" 2> "$WORK/err.txt" \
    || fail "turno con metacaracteres terminó con error"
[ ! -e "$WORK/CANARIO" ] || fail "se ejecutó texto transcrito como comando"
grep -q 'touch' "$WORK/prompt.txt" || fail "el texto debía llegar literal al prompt"

# 6) Configuración inválida ---------------------------------------------------------
JARVIS_LLAMA_BIN=/ruta/que/no/existe "$BIN" --check 2> "$WORK/err.txt"
[ $? -eq 2 ] || fail "configuración inválida debía terminar con código 2"
"$BIN" --check 2> /dev/null || fail "configuración válida debía pasar --check"
"$BIN" --turns 0 2> /dev/null
[ $? -eq 2 ] || fail "--turns 0 debía rechazarse"

echo "OK: prueba de extremo a extremo con stubs"
