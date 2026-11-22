#pragma once

namespace jarvis {

// Estados del orquestador. Listo -> Grabando -> Transcribiendo -> Generando
// -> Hablando -> Listo. Cualquier etapa puede pasar a Error y luego a Listo.
enum class State { Ready, Recording, Transcribing, Generating, Speaking, Error };

const char* state_name(State s);

}  // namespace jarvis

