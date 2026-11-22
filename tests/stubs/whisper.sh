#!/bin/sh
# Simula whisper-cli. Comprueba que el WAV (-f) exista, como haría el real.
wav=""
while [ $# -gt 0 ]; do
    case "$1" in
        -f) wav="$2"; shift ;;
    esac
    shift
done
[ -f "$wav" ] || exit 1
[ -n "$JARVIS_STUB_WHISPER_FAIL" ] && exit 4
if [ -n "$JARVIS_STUB_WHISPER_EMPTY" ]; then
    echo " [BLANK_AUDIO]"
    exit 0
fi
printf '\n %s\n' "${JARVIS_STUB_WHISPER_TEXT:-Cuál es la capital de Francia.}"
