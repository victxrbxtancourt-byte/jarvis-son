#pragma once

#include <csignal>
#include <cstddef>
#include <string>
#include <vector>

namespace jarvis {

struct RunOptions {
    // argv[0] se resuelve con execvp: si contiene '/', se usa tal cual.
    // Los argumentos se pasan sin shell, así que no hay inyección de comandos.
    std::vector<std::string> argv;
    std::string stdin_data;                 // vacío -> stdin sin datos
    int timeout_s = 60;
    std::size_t max_output = 64 * 1024;     // tope de bytes capturados de stdout
};

struct RunResult {
    bool started = false;
    int exit_code = -1;       // código de salida (si terminó normalmente)
    int term_signal = 0;      // señal que lo mató (por ejemplo 9 = SIGKILL)
    bool timed_out = false;
    bool interrupted = false; // el orquestador recibió SIGINT/SIGTERM
    std::string out;          // stdout (stderr se descarta)
    double seconds = 0.0;
    long max_rss_kb = 0;      // memoria residente máxima del proceso hijo
    std::string error;        // fallo al lanzar (fork, pipes)

    bool ok() const {
        return started && !timed_out && !interrupted && term_signal == 0 && exit_code == 0;
    }
};

// Lanza el proceso, le entrega stdin_data, captura stdout y espera su fin.
// Mata el grupo de procesos si se agota el tiempo o si *stop_flag se activa.
RunResult run_process(const RunOptions& opt, const volatile std::sig_atomic_t* stop_flag = nullptr);

// Descripción corta de un fallo, para mensajes al usuario.
std::string describe_failure(const RunResult& r);

}  // namespace jarvis