#!/bin/sh
# Simula llama-completion. Guarda el prompt (-p) para que la prueba lo revise.
prompt=""
while [ $# -gt 0 ]; do
    case "$1" in
        -p) prompt="$2"; shift ;;
    esac
    shift
done
if [ -n "$JARVIS_STUB_DIR" ]; then
    printf '%s' "$prompt" > "$JARVIS_STUB_DIR/prompt.txt"
    : > "$JARVIS_STUB_DIR/llama_called"
fi
[ -n "$JARVIS_STUB_LLAMA_KILL" ] && kill -9 $$
printf 'La capital de Francia es París. [end of text]\n'
