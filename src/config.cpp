#include "jarvis/config.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <cstdlib>

namespace jarvis {

namespace {

std::string env_or(const char* name, const std::string& fallback) {
    const char* v = std::getenv(name);
    return (v != nullptr && *v != '\0') ? std::string(v) : fallback;
}

void check_path(std::vector<std::string>& problems, const char* label, const std::string& path,
                int mode, const char* mode_name) {
    if (path.empty() || path.find('/') == std::string::npos) {
        problems.push_back(std::string(label) + ": debe ser una ruta con '/' (valor: '" + path +
                           "')");
        return;
    }
    if (access(path.c_str(), mode) != 0)
        problems.push_back(std::string(label) + ": no existe o no tiene permiso de " + mode_name +
                           ": " + path);
}

void check_range(std::vector<std::string>& problems, const char* label, int v, int lo, int hi) {
    if (v < lo || v > hi)
        problems.push_back(std::string(label) + ": " + std::to_string(v) + " fuera del rango " +
                           std::to_string(lo) + "-" + std::to_string(hi));
}

}  // namespace

Config config_from_env() {
    Config c;
    const std::string home = env_or("HOME", "");
    c.capture_script = env_or("JARVIS_CAPTURE", c.capture_script);
    c.tts_script = env_or("JARVIS_TTS", c.tts_script);
    c.whisper_bin = env_or("JARVIS_WHISPER_BIN", home + "/whisper.cpp/build/bin/whisper-cli");
    c.whisper_model = env_or("JARVIS_WHISPER_MODEL", home + "/whisper.cpp/models/ggml-base.bin");
    c.llama_bin = env_or("JARVIS_LLAMA_BIN", home + "/llama.cpp/build/bin/llama-completion");
    c.llama_model = env_or("JARVIS_LLAMA_MODEL", home + "/models/Qwen3.5-0.8B-Q4_0.gguf");
    c.tmp_base = env_or("JARVIS_TMPDIR", env_or("TMPDIR", "/tmp"));
    return c;
}

std::vector<std::string> validate_config(const Config& c) {
    std::vector<std::string> p;
    check_path(p, "captura", c.capture_script, X_OK, "ejecución");
    check_path(p, "tts", c.tts_script, X_OK, "ejecución");
    check_path(p, "whisper-cli", c.whisper_bin, X_OK, "ejecución");
    check_path(p, "modelo de STT", c.whisper_model, R_OK, "lectura");
    check_path(p, "llama-completion", c.llama_bin, X_OK, "ejecución");
    check_path(p, "modelo del SLM", c.llama_model, R_OK, "lectura");

    struct stat st;
    if (stat(c.tmp_base.c_str(), &st) != 0 || !S_ISDIR(st.st_mode) ||
        access(c.tmp_base.c_str(), W_OK) != 0)
        p.push_back("directorio temporal: no existe o no se puede escribir: " + c.tmp_base);

    check_range(p, "segundos de grabación", c.record_seconds, 1, 30);
    check_range(p, "hilos", c.threads, 1, 16);
    check_range(p, "contexto", c.ctx, 256, 4096);
    check_range(p, "tokens de salida", c.n_predict, 16, 256);
    return p;
}

}  // namespace jarvis
