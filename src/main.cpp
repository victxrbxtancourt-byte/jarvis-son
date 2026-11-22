// jarvis-son: orquestador de la V1 (captura -> STT -> SLM -> TTS), por turnos.
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "jarvis/config.hpp"
#include "jarvis/orchestrator.hpp"
#include "jarvis/state.hpp"

namespace fs = std::filesystem;

namespace {

volatile std::sig_atomic_t g_stop = 0;

void on_signal(int) { g_stop = 1; }

void install_signal_handlers() {
    struct sigaction sa;
    std::memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

void usage(const char* prog) {
    std::fprintf(stderr,
                 "Uso: %s [opciones]\n"
                 "  --turns N       número de turnos consecutivos (1-100, por defecto 1)\n"
                 "  --seconds N     segundos de grabación por turno (1-30, por defecto 5)\n"
                 "  --threads N     hilos para STT y SLM (1-16, por defecto 4)\n"
                 "  --log ARCHIVO   agrega tiempos y memoria por etapa a un CSV (sin texto)\n"
                 "  --show-text     muestra la transcripción y la respuesta en pantalla\n"
                 "  --check         solo comprueba rutas y parámetros, sin ejecutar nada\n"
                 "  --help          muestra esta ayuda\n"
                 "\n"
                 "Rutas por variables de entorno: JARVIS_CAPTURE, JARVIS_TTS,\n"
                 "JARVIS_WHISPER_BIN, JARVIS_WHISPER_MODEL, JARVIS_LLAMA_BIN,\n"
                 "JARVIS_LLAMA_MODEL, JARVIS_TMPDIR.\n",
                 prog);
}

bool parse_int(const char* text, int lo, int hi, int& out) {
    if (text == nullptr || *text == '\0') return false;
    char* end = nullptr;
    const long v = std::strtol(text, &end, 10);
    if (*end != '\0' || v < lo || v > hi) return false;
    out = static_cast<int>(v);
    return true;
}

void append_log(const std::string& path, int turn, const jarvis::TurnResult& r) {
    std::error_code ec;
    const bool write_header = !fs::exists(path, ec) || fs::file_size(path, ec) == 0;
    std::ofstream f(path, std::ios::app);
    if (!f) {
        std::fprintf(stderr, "AVISO: no se pudo escribir el registro %s\n", path.c_str());
        return;
    }
    if (write_header)
        f << "turno;resultado;fallo;captura_s;captura_ram_mb;stt_s;stt_ram_mb;"
             "slm_s;slm_ram_mb;tts_s;tts_ram_mb;total_s\n";
    char line[256];
    std::snprintf(line, sizeof line, "%d;%s;%s;%.2f;%.0f;%.2f;%.0f;%.2f;%.0f;%.2f;%.0f;%.2f\n",
                  turn, r.ok ? "ok" : "error", r.ok ? "-" : jarvis::state_name(r.failed_at),
                  r.capture.seconds, r.capture.ram_mb, r.stt.seconds, r.stt.ram_mb, r.slm.seconds,
                  r.slm.ram_mb, r.tts.seconds, r.tts.ram_mb, r.total_seconds);
    f << line;
}

}  // namespace

int main(int argc, char** argv) {
    jarvis::Config cfg = jarvis::config_from_env();
    int turns = 1;
    bool show_text = false;
    bool check_only = false;
    std::string log_path;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        const char* next = (i + 1 < argc) ? argv[i + 1] : nullptr;
        if (a == "--help" || a == "-h") {
            usage(argv[0]);
            return 0;
        } else if (a == "--show-text") {
            show_text = true;
        } else if (a == "--check") {
            check_only = true;
        } else if (a == "--turns" || a == "--seconds" || a == "--threads") {
            int v = 0;
            const int lo = 1;
            const int hi = (a == "--turns") ? 100 : (a == "--seconds") ? 30 : 16;
            if (!parse_int(next, lo, hi, v)) {
                std::fprintf(stderr, "ERROR: %s necesita un entero entre %d y %d\n", a.c_str(), lo,
                             hi);
                return 2;
            }
            if (a == "--turns")
                turns = v;
            else if (a == "--seconds")
                cfg.record_seconds = v;
            else
                cfg.threads = v;
            ++i;
        } else if (a == "--log") {
            if (next == nullptr || *next == '\0') {
                std::fprintf(stderr, "ERROR: --log necesita una ruta\n");
                return 2;
            }
            log_path = next;
            ++i;
        } else {
            std::fprintf(stderr, "ERROR: opción desconocida: %s\n\n", a.c_str());
            usage(argv[0]);
            return 2;
        }
    }

    const auto problems = jarvis::validate_config(cfg);
    if (!problems.empty()) {
        std::fprintf(stderr, "Configuración inválida:\n");
        for (const auto& p : problems) std::fprintf(stderr, "  - %s\n", p.c_str());
        return 2;
    }
    if (check_only) {
        std::fprintf(stderr, "Configuración correcta.\n");
        return 0;
    }

    install_signal_handlers();

    jarvis::Orchestrator orch(cfg, &g_stop, [](jarvis::State s) {
        std::fprintf(stderr, "[estado] %s\n", jarvis::state_name(s));
    });

    int failures = 0;
    for (int t = 1; t <= turns && g_stop == 0; ++t) {
        std::fprintf(stderr, "\n=== Turno %d/%d ===\n", t, turns);
        const jarvis::TurnResult r = orch.run_turn();

        if (show_text) {
            if (!r.transcript.empty()) std::printf("Tú: %s\n", r.transcript.c_str());
            if (!r.response.empty()) std::printf("Jarvis-son: %s\n", r.response.c_str());
        }
        if (r.ok) {
            std::fprintf(
                stderr,
                "[tiempos] captura=%.2fs stt=%.2fs slm=%.2fs tts=%.2fs total=%.2fs\n"
                "[memoria] captura=%.0fMB stt=%.0fMB slm=%.0fMB tts=%.0fMB (pico por proceso)\n",
                r.capture.seconds, r.stt.seconds, r.slm.seconds, r.tts.seconds, r.total_seconds,
                r.capture.ram_mb, r.stt.ram_mb, r.slm.ram_mb, r.tts.ram_mb);
        } else {
            ++failures;
            std::fprintf(stderr, "[fallo en %s] %s\n", jarvis::state_name(r.failed_at),
                         r.message.c_str());
        }
        if (!log_path.empty()) append_log(log_path, t, r);
    }

    if (g_stop != 0) {
        std::fprintf(stderr, "\nInterrumpido.\n");
        return 130;
    }
    std::fprintf(stderr, "\nTurnos correctos: %d de %d\n", turns - failures, turns);
    return failures == 0 ? 0 : 1;
}
