#!/bin/sh
# Simula scripts/capture.sh: escribe un "WAV" falso en la ruta recibida ($2).
[ -n "$2" ] || exit 1
head -c 2000 /dev/zero > "$2"
echo "OK: $2"
