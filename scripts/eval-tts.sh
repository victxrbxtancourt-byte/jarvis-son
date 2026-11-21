#!/data/data/com.termux/files/usr/bin/bash
# Reproduce cada frase y registra duración y puntuación de pronunciación y naturalidad.
# Uso: ./scripts/eval-tts.sh [frases.txt] [resultados.csv]
# Variable opcional: TTS_ENGINE
set -euo pipefail

FRASES="${1:-docs/tts-frases.txt}"
OUT="${2:-docs/tts-resultados.csv}"
[ -f "$FRASES" ] || { echo "ERROR: no existe $FRASES"; exit 1; }

echo "n;frase;segundos;pronunciacion_1a5;naturalidad_1a5;observaciones" > "$OUT"

n=0
while IFS= read -r -u 3 frase || [ -n "$frase" ]; do
  [ -z "$frase" ] && continue
  n=$((n + 1))
  echo
  echo "Frase $n: $frase"
  read -r -p "Enter para reproducir... " _

  START="$(date +%s.%N)"
  ./scripts/tts.sh "$frase"
  END="$(date +%s.%N)"
  SEC="$(awk -v a="$START" -v b="$END" 'BEGIN { printf "%.2f", b - a }')"

  read -r -p "Pronunciación (1-5): " PRON
  read -r -p "Naturalidad (1-5): " NAT
  read -r -p "Observaciones (opcional): " OBS

  F="$(echo "$frase" | tr ';' ',')"
  OBS="$(echo "$OBS" | tr ';' ',')"
  echo "$n;$F;$SEC;$PRON;$NAT;$OBS" >> "$OUT"
done 3< "$FRASES"

echo
awk -F';' 'NR>1 { p+=$4; t+=$5; s+=$3; n++ } END { if (n) printf "Promedios: pronunciación %.1f, naturalidad %.1f, duración %.2f s (n=%d)\n", p/n, t/n, s/n, n }' "$OUT"
echo "Guardado en $OUT"