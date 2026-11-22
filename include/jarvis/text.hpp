#pragma once

#include <cstddef>
#include <string>

namespace jarvis {

// Limpieza del texto que viaja entre etapas. Ninguna de estas funciones
// construye comandos: solo normalizan cadenas.

// Recorta y colapsa espacios; los caracteres de control cuentan como espacio.
std::string normalize_spaces(const std::string& in);

// Corta a max_bytes sin partir un carácter UTF-8.
std::string truncate_utf8(const std::string& s, std::size_t max_bytes);

// Salida de STT -> texto para el SLM. Quita marcadores de Whisper entre
// corchetes o paréntesis (por ejemplo "[BLANK_AUDIO]"), tokens especiales del
// formato de chat y espacios sobrantes. Devuelve "" si no queda contenido.
std::string clean_transcript(const std::string& raw, std::size_t max_bytes);

// Salida del SLM -> texto para el TTS. Quita "[end of text]", tokens
// especiales y etiquetas de razonamiento, y normaliza espacios.
std::string clean_response(const std::string& raw, std::size_t max_bytes);

// Prompt en formato ChatML con bloque <think> vacío (sin razonamiento visible).
std::string build_prompt(const std::string& transcript);

}  // namespace jarvis