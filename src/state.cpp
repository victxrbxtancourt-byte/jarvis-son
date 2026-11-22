#include "jarvis/state.hpp"

namespace jarvis {

const char* state_name(State s) {
    switch (s) {
        case State::Ready: return "Listo";
        case State::Recording: return "Grabando";
        case State::Transcribing: return "Transcribiendo";
        case State::Generating: return "Generando";
        case State::Speaking: return "Hablando";
        case State::Error: return "Error";
    }
    return "Desconocido";
}

}  // namespace jarvis
