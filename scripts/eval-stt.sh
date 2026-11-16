#!/data/data/com.termux/files/usr/bin/bash
# Evalúa el STT frase por frase: graba, transcribe, mide tiempo y memoria pico.
# Uso: ./scripts/eval-stt.sh [frases.txt] [resultados.csv]
# Variables opcionales: WHISPER_DIR, MODEL, THREADS
set -euo pipefail

WHISPER_DIR="${WHISPER_DIR:-$HOME/whisper.cpp}"
MODEL="${MODEL:-$WHISPER_DIR/models/ggml-tiny.bin}"
THREADS="${THREADS:-4}"
FRASES="${1:-docs/stt-frases.txt}"
OUT="${2:-docs/stt-resultados.csv}"
CLI="$WHISPER_DIR/build/bin/whisper-cli"
TIMEBIN="$(type -P time || true)"

[ -x "$CLI" ]      || { echo "ERROR: no existe $CLI (compila whisper.cpp)"; exit 1; }
[ -f "$MODEL" ]    || { echo "ERROR: no existe el modelo $MODEL"; exit 1; }
[ -n "$TIMEBIN" ]  || { echo "ERROR: falta el paquete time (pkg install time)"; exit 1; }
[ -f "$FRASES" ]   || { echo "ERROR: no existe $FRASES"; exit 1; }

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "n;referencia;transcripcion;segundos;ram_pico_mb;utilizable" > "$OUT"

n=0
ok=0
while IFS= read -r -u 3 frase || [ -n "$frase" ]; do
  [ -z "$frase" ] && continue
  n=$((n + 1))
  echo
  echo "Frase $n: $frase"
  read -r -p "Enter para grabar (5 s)... " _
  ./scripts/capture.sh 5 "$TMP/clip.wav" >/dev/null

  "$TIMEBIN" -f "%e %M" -o "$TMP/time.txt" \
    "$CLI" -m "$MODEL" -f "$TMP/clip.wav" -l es -t "$THREADS" -nt -np \
    > "$TMP/out.txt" 2>/dev/null || true

  read -r SEC KB < <(tail -n 1 "$TMP/time.txt")
  MB=$((KB / 1024))
  TXT="$(tr '\n' ' ' < "$TMP/out.txt" | tr ';' ',' | sed 's/^ *//; s/ *$//')"

  echo "Transcripción: $TXT"
  echo "Tiempo: ${SEC} s | RAM pico: ${MB} MB"
  read -r -p "¿Utilizable? (s/n): " U
  case "$U" in [sS]*) U="si"; ok=$((ok + 1)) ;; *) U="no" ;; esac

  REF="$(echo "$frase" | tr ';' ',')"
  echo "$n;$REF;$TXT;$SEC;$MB;$U" >> "$OUT"
done 3< "$FRASES"

echo
echo "Resultado: $ok de $n transcripciones utilizables. Guardado en $OUT"