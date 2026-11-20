#!/data/data/com.termux/files/usr/bin/bash
# Evalúa el SLM pregunta por pregunta: genera, mide tiempo, memoria pico y velocidad.
# Uso: ./scripts/eval-slm.sh [preguntas.txt] [resultados.csv]
# Variables opcionales: LLAMA_DIR, MODEL, THREADS, CTX, NPRED, SEED
set -euo pipefail

LLAMA_DIR="${LLAMA_DIR:-$HOME/llama.cpp}"
MODEL="${MODEL:-$HOME/models/Qwen3.5-0.8B-Q4_0.gguf}"
THREADS="${THREADS:-4}"
CTX="${CTX:-1024}"
NPRED="${NPRED:-96}"
SEED="${SEED:-42}"
PREGUNTAS="${1:-docs/slm-preguntas.txt}"
OUT="${2:-docs/slm-resultados.csv}"
BIN="$LLAMA_DIR/build/bin/llama-completion"
TIMEBIN="$(type -P time || true)"
SYSTEM="Eres Jarvis-son, un asistente de voz. Responde siempre en español latinoamericano, en una o dos frases cortas, sin listas ni formato."

[ -x "$BIN" ]       || { echo "ERROR: no existe $BIN (compila llama.cpp)"; exit 1; }
[ -f "$MODEL" ]     || { echo "ERROR: no existe el modelo $MODEL"; exit 1; }
[ -n "$TIMEBIN" ]   || { echo "ERROR: falta el paquete time (pkg install time)"; exit 1; }
[ -f "$PREGUNTAS" ] || { echo "ERROR: no existe $PREGUNTAS"; exit 1; }

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "n;pregunta;respuesta;segundos;ram_pico_mb;tokens_por_s;adecuada" > "$OUT"

n=0
ok=0
while IFS= read -r -u 3 pregunta || [ -n "$pregunta" ]; do
  [ -z "$pregunta" ] && continue
  n=$((n + 1))

  PROMPT=$'<|im_start|>system\n'"$SYSTEM"$'<|im_end|>\n<|im_start|>user\n'"$pregunta"$'<|im_end|>\n<|im_start|>assistant\n<think>\n\n</think>\n\n'

  "$TIMEBIN" -f "%e %M" -o "$TMP/time.txt" \
    "$BIN" -m "$MODEL" -c "$CTX" -n "$NPRED" -t "$THREADS" --seed "$SEED" \
      --temp 0.7 --top-k 20 --top-p 0.8 -no-cnv --no-display-prompt --perf \
      -p "$PROMPT" < /dev/null > "$TMP/out.txt" 2> "$TMP/err.txt" || true

  read -r SEC KB < <(tail -n 1 "$TMP/time.txt")
  MB=$((KB / 1024))
  RESP="$(tr '\n;' '  ' < "$TMP/out.txt" | sed 's/\[end of text\]//; s/  */ /g; s/^ //; s/ $//')"
  TOKS="$(grep -E 'llama_perf_context_print: +eval time' "$TMP/err.txt" \
          | sed -E 's/.*, *([0-9.]+) tokens per second.*/\1/' | head -n 1 || true)"

  echo
  echo "Pregunta $n: $pregunta"
  echo "Respuesta: $RESP"
  echo "Tiempo: ${SEC} s | RAM pico: ${MB} MB | Velocidad: ${TOKS:-?} tok/s"
  read -r -p "¿Adecuada? (s/n): " U
  case "$U" in [sS]*) U="si"; ok=$((ok + 1)) ;; *) U="no" ;; esac

  PREG="$(echo "$pregunta" | tr ';' ',')"
  echo "$n;$PREG;$RESP;$SEC;$MB;${TOKS:-};$U" >> "$OUT"
done 3< "$PREGUNTAS"

echo
echo "Resultado: $ok de $n respuestas adecuadas. Guardado en $OUT"