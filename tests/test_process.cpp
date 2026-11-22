#include <csignal>
#include <string>

#include "check.hpp"
#include "jarvis/process.hpp"

using namespace jarvis;

namespace {
RunResult run(std::vector<std::string> argv, const std::string& in = "", int timeout = 10,
              std::size_t cap = 64 * 1024, const volatile std::sig_atomic_t* stop = nullptr) {
    RunOptions o;
    o.argv = std::move(argv);
    o.stdin_data = in;
    o.timeout_s = timeout;
    o.max_output = cap;
    return run_process(o, stop);
}
}  // namespace

int main() {
    // Salida normal y código 0
    {
        const RunResult r = run({"echo", "hola"});
        CHECK(r.ok());
        CHECK_EQ(r.out, std::string("hola\n"));
        CHECK(r.max_rss_kb > 0);
    }

    // stdin llega al proceso (así viaja el texto al TTS)
    {
        const RunResult r = run({"cat"}, "texto por stdin");
        CHECK(r.ok());
        CHECK_EQ(r.out, std::string("texto por stdin"));
    }

    // Texto con metacaracteres de shell: se pasa como argumento literal, sin shell
    {
        const std::string raro = "$(echo pwned); `id` && rm -rf / | \"'";
        const RunResult r = run({"printf", "%s", raro});
        CHECK(r.ok());
        CHECK_EQ(r.out, raro);
    }

    // Código de salida distinto de cero
    {
        const RunResult r = run({"sh", "-c", "exit 3"});
        CHECK(!r.ok());
        CHECK_EQ(r.exit_code, 3);
        CHECK_EQ(r.term_signal, 0);
    }

    // Muerte por señal 9 (lo que pasa cuando Android mata por falta de memoria)
    {
        const RunResult r = run({"sh", "-c", "kill -9 $$"});
        CHECK(!r.ok());
        CHECK_EQ(r.term_signal, SIGKILL);
        CHECK(describe_failure(r).find("señal 9") != std::string::npos);
    }

    // Programa inexistente: el hijo sale con 127
    {
        const RunResult r = run({"programa-que-no-existe-jarvis"});
        CHECK(!r.ok());
        CHECK_EQ(r.exit_code, 127);
    }

    // Tiempo agotado: se mata el proceso y también a sus descendientes
    {
        const RunResult r = run({"sh", "-c", "sleep 30 & sleep 30"}, "", 1);
        CHECK(r.timed_out);
        CHECK(!r.ok());
        CHECK(r.seconds < 5.0);
    }

    // Tope de salida: el proceso no se bloquea aunque escriba mucho
    {
        const RunResult r = run({"sh", "-c", "yes x | head -c 200000"}, "", 10, 1000);
        CHECK(r.ok());
        CHECK_EQ(r.out.size(), static_cast<std::size_t>(1000));
    }

    // Entrada grande por stdin (más que el buffer de la tubería): sin interbloqueo
    {
        const std::string grande(300000, 'z');
        const RunResult r = run({"cat"}, grande, 10, 400000);
        CHECK(r.ok());
        CHECK_EQ(r.out.size(), grande.size());
    }

    // Interrupción por bandera (SIGINT/SIGTERM del usuario)
    {
        static volatile std::sig_atomic_t stop = 1;
        const RunResult r = run({"sleep", "30"}, "", 30, 1024, &stop);
        CHECK(r.interrupted);
        CHECK(!r.ok());
        CHECK(r.seconds < 5.0);
    }

    // Comando vacío
    {
        const RunResult r = run({});
        CHECK(!r.started);
        CHECK(!r.ok());
    }

    return TEST_RESULT();
}
