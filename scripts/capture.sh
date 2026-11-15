#!/data/data/com.termux/files/usr/bin/bash
# Captura un clip corto, lo normaliza a WAV mono 16 kHz y valida el resultado.
# Uso: ./scripts/capture.sh [segundos] [salida.wav]
set -euo pipefail

SECS="${1:-6}"
OUT="${2:-clip.wav}"

# Validar argumentos
case "$SECS" in
    ''|*[!0-9]*) echo "ERROR: los segundos deben ser un número entero"; exit 1 ;;
esac
if [ "$SECS" -lt 1 ] || [ "$SECS" -gt 30 ]; then
    echo "ERROR: usa entre 1 y 30 segundos"; exit 1
fi

# Temporales: siempre se borran al terminar, incluso si algo falla
TMP="$(mktemp -d)"
cleanup() {
    termux-microphone-record -q >/dev/null 2>&1 || true
    rm -rf "$TMP"
}
trap cleanup EXIT

RAW="$TMP/raw.m4a"

# Si había una grabación activa, detenerla
termux-microphone-record -q >/dev/null 2>&1 || true

echo "Grabando ${SECS} s... habla ahora"
termux-microphone-record -f "$RAW" -l "$SECS" >/dev/null
sleep "$((SECS + 1))"

[ -s "$RAW" ] || { echo "ERROR: no se grabó audio (revisa el permiso de micrófono)"; exit 1; }

# Normalizar: mono, 16 kHz, PCM 16 bits
ffmpeg -y -loglevel error -i "$RAW" -ac 1 -ar 16000 -c:a pcm_s16le "$OUT"

# Validar formato
INFO="$(ffprobe -v error -select_streams a:0 \
    -show_entries stream=codec_name,sample_rate,channels \
    -of default=noprint_wrappers=1 "$OUT")"

echo "$INFO" | grep -q 'codec_name=pcm_s16le' \
    && echo "$INFO" | grep -q 'sample_rate=16000' \
    && echo "$INFO" | grep -q 'channels=1' \
    || { echo "ERROR: formato inválido"; echo "$INFO"; rm -f "$OUT"; exit 1; }

# Validar duración
DUR="$(ffprobe -v error -show_entries format=duration -of csv=p=0 "$OUT")"
awk -v d="$DUR" 'BEGIN { exit !(d >= 0.5) }' \
    || { echo "ERROR: clip demasiado corto (${DUR}s)"; rm -f "$OUT"; exit 1; }

echo "OK: $OUT (WAV mono, 16 kHz, ${DUR}s)"