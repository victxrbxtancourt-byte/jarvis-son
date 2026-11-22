#!/data/data/com.termux/files/usr/bin/bash
# Resume el CSV que genera `jarvis-son --log`: turnos correctos y, por etapa,
# p50, p95, máximo de tiempo y memoria pico. Solo usa los turnos correctos.
# Uso: ./scripts/summarize-turns.sh docs/turnos-v1.csv
set -euo pipefail

CSV="${1:-docs/turnos-v1.csv}"
[ -f "$CSV" ] || { echo "ERROR: no existe $CSV"; exit 1; }

TOTAL="$(awk -F';' 'NR>1 { n++ } END { print n+0 }' "$CSV")"
OK="$(awk -F';' 'NR>1 && $2=="ok" { n++ } END { print n+0 }' "$CSV")"
[ "$OK" -gt 0 ] || { echo "ERROR: no hay turnos correctos en $CSV"; exit 1; }

# Percentil por rango más cercano sobre los valores de una columna.
percentile() {  # columna, percentil
    local col="$1" p="$2" n rank
    n="$OK"
    rank=$(((p * n + 99) / 100))
    awk -F';' -v c="$col" 'NR>1 && $2=="ok" { print $c }' "$CSV" | sort -n | sed -n "${rank}p"
}
maximum() {
    awk -F';' -v c="$1" 'NR>1 && $2=="ok" { if (!seen || $c > m) { m = $c; seen = 1 } } END { print m }' "$CSV"
}

echo "Turnos: $TOTAL (correctos: $OK, con error: $((TOTAL - OK)))"
echo
echo "| Etapa | p50 (s) | p95 (s) | máx (s) | memoria pico (MB) |"
echo "|---|---|---|---|---|"
row() {  # nombre, columna de tiempo, columna de memoria (0 = sin memoria)
    local ram="-"
    [ "$3" -gt 0 ] && ram="$(maximum "$3")"
    echo "| $1 | $(percentile "$2" 50) | $(percentile "$2" 95) | $(maximum "$2") | $ram |"
}
row "Captura" 4 5
row "STT" 6 7
row "SLM" 8 9
row "TTS" 10 11
row "Turno completo" 12 0
echo
echo "Percentiles por rango más cercano sobre $OK turnos correctos."
