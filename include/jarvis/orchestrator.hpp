#pragma once

#include <csignal>
#include <functional>
#include <string>

#include "jarvis/config.hpp"
#include "jarvis/state.hpp"

namespace jarvis {

struct StageStats {
    double seconds = 0.0;
    double ram_mb = 0.0;  // memoria residente máxima del proceso de la etapa
};

struct TurnResult {
    bool ok = false;
    State failed_at = State::Ready;  // etapa en la que falló (si ok == false)
    std::string message;             // motivo del fallo
    StageStats capture, stt, slm, tts;
    double total_seconds = 0.0;

    // Solo en memoria, para mostrarlos con --show-text. Nunca se escriben a disco.
    std::string transcript;
    std::string response;
};

// Orquesta un turno: Listo -> Grabando -> Transcribiendo -> Generando ->
// Hablando -> Listo (o Error -> Listo). Cada etapa es un proceso aparte, con
// argumentos fijos, y termina antes de que empiece la siguiente.
class Orchestrator {
public:
    using StateCallback = std::function<void(State)>;

    Orchestrator(Config cfg, const volatile std::sig_atomic_t* stop_flag, StateCallback on_state);

    TurnResult run_turn();

private:
    Config cfg_;
    const volatile std::sig_atomic_t* stop_;
    StateCallback on_state_;
};

}  // namespace jarvis

