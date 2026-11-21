#!/data/data/com.termux/files/usr/bin/bash
# Dice un texto en voz alta con Termux:API.
# Uso: ./scripts/tts.sh "texto"   o   echo "texto" | ./scripts/tts.sh
# Variables opcionales: TTS_ENGINE (nombre del motor), TTS_LANG (es), TTS_REGION (MX)
set -euo pipefail

MAXLEN=400

if [ $# -ge 1 ]; then TEXT="$1"; else TEXT="$(cat)"; fi

# Quitar caracteres de control y normalizar espacios
TEXT="$(printf '%s' "$TEXT" | tr -d '\000-\010\013-\037' | tr '\n\t' '  ' | sed 's/  */ /g; s/^ //; s/ $//')"

[ -n "$TEXT" ] || { echo "ERROR: texto vacío"; exit 1; }
if [ "${#TEXT}" -gt "$MAXLEN" ]; then TEXT="${TEXT:0:$MAXLEN}"; fi

ARGS=(-l "${TTS_LANG:-es}" -n "${TTS_REGION:-MX}")
if [ -n "${TTS_ENGINE:-}" ]; then ARGS+=(-e "$TTS_ENGINE"); fi

printf '%s' "$TEXT" | termux-tts-speak "${ARGS[@]}"