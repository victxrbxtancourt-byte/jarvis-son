#pragma once

#include <string>
#include <vector>

namespace jarvis {

struct Config {
    // Rutas. Todas deben contener '/' para que nunca se busquen en PATH.
    std::string capture_script = "scripts/capture.sh";
    std::string tts_script = "scripts/tts.sh";
    std::string whisper_bin;
    std::string whisper_model;
    std::string llama_bin;
    std::string llama_model;
    std::string tmp_base;

    // Parámetros fijos del proyecto (ver docs/interfaces.md).
    int record_seconds = 5;
    int threads = 4;
    int ctx = 1024;
    int n_predict = 96;
    int seed = 42;

    // Tiempos máximos por etapa, en segundos.
    int timeout_capture_extra = 30;  // se suma a record_seconds
    int timeout_stt = 90;
    int timeout_slm = 180;
    int timeout_tts = 90;
};

// Valores por defecto del equipo de pruebas, con sobreescritura por variables
// de entorno: JARVIS_CAPTURE, JARVIS_TTS, JARVIS_WHISPER_BIN,
// JARVIS_WHISPER_MODEL, JARVIS_LLAMA_BIN, JARVIS_LLAMA_MODEL, JARVIS_TMPDIR.
Config config_from_env();

// Devuelve la lista de problemas encontrados (vacía si todo está bien).
std::vector<std::string> validate_config(const Config& cfg);

}  // namespace jarvis
