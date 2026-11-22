#include "jarvis/orchestrator.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <system_error>
#include <utility>
#include <vector>

#include "jarvis/process.hpp"
#include "jarvis/text.hpp"

namespace fs = std::filesystem;

namespace jarvis {

namespace {

using Clock = std::chrono::steady_clock;

constexpr std::size_t kMaxTranscriptBytes = 300;  // acota el prompt (contexto de 1.024)
constexpr std::size_t kMaxResponseBytes = 400;    // mismo tope que scripts/tts.sh

// Directorio temporal privado del turno. Se borra siempre al destruirse, con
// éxito, con error o por excepción: el turno no deja audios.
class TempDir {
public:
    explicit TempDir(const std::string& base) {
        std::string tmpl = base + "/jarvis-XXXXXX";
        std::vector<char> buf(tmpl.begin(), tmpl.end());
        buf.push_back('\0');
        if (mkdtemp(buf.data()) != nullptr) path_ = buf.data();
    }
    ~TempDir() {
        if (!path_.empty()) {
            std::error_code ec;
            fs::remove_all(path_, ec);
        }
    }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    bool ok() const { return !path_.empty(); }
    const std::string& path() const { return path_; }

private:
    std::string path_;
};

double ram_mb(const RunResult& r) { return static_cast<double>(r.max_rss_kb) / 1024.0; }

}  // namespace

Orchestrator::Orchestrator(Config cfg, const volatile std::sig_atomic_t* stop_flag,
                           StateCallback on_state)
    : cfg_(std::move(cfg)), stop_(stop_flag), on_state_(std::move(on_state)) {}

TurnResult Orchestrator::run_turn() {
    TurnResult res;
    const Clock::time_point t0 = Clock::now();

    auto enter = [&](State s) {
        if (on_state_) on_state_(s);
    };
    auto finish = [&]() {
        res.total_seconds = std::chrono::duration<double>(Clock::now() - t0).count();
        enter(State::Ready);
    };
    auto fail = [&](State at, const std::string& msg) {
        res.ok = false;
        res.failed_at = at;
        res.message = msg;
        enter(State::Error);
        finish();
        return res;
    };

    TempDir tmp(cfg_.tmp_base);
    if (!tmp.ok()) return fail(State::Recording, "no se pudo crear el directorio temporal");
    const std::string wav = tmp.path() + "/turno.wav";

    // 1) Captura -----------------------------------------------------------
    enter(State::Recording);
    {
        RunOptions o;
        o.argv = {cfg_.capture_script, std::to_string(cfg_.record_seconds), wav};
        o.timeout_s = cfg_.record_seconds + cfg_.timeout_capture_extra;
        const RunResult rr = run_process(o, stop_);
        res.capture = {rr.seconds, ram_mb(rr)};
        if (!rr.ok()) return fail(State::Recording, "captura: " + describe_failure(rr));

        std::error_code ec;
        const auto size = fs::file_size(wav, ec);
        if (ec || size <= 44) return fail(State::Recording, "captura: no se generó un WAV válido");
    }

    // 2) STT ---------------------------------------------------------------
    enter(State::Transcribing);
    {
        RunOptions o;
        o.argv = {cfg_.whisper_bin,
                  "-m",
                  cfg_.whisper_model,
                  "-f",
                  wav,
                  "-l",
                  "es",
                  "-t",
                  std::to_string(cfg_.threads),
                  "-nt",
                  "-np"};
        o.timeout_s = cfg_.timeout_stt;
        const RunResult rr = run_process(o, stop_);
        res.stt = {rr.seconds, ram_mb(rr)};

        std::error_code ec;
        fs::remove(wav, ec);  // el audio ya no se necesita: se borra de inmediato

        if (!rr.ok()) return fail(State::Transcribing, "STT: " + describe_failure(rr));
        res.transcript = clean_transcript(rr.out, kMaxTranscriptBytes);
        if (res.transcript.empty())
            return fail(State::Transcribing, "STT: no se entendió nada (transcripción vacía)");
    }

    // 3) SLM ---------------------------------------------------------------
    enter(State::Generating);
    {
        RunOptions o;
        // El texto transcrito viaja como UN argumento de execvp, sin shell.
        o.argv = {cfg_.llama_bin,
                  "-m",
                  cfg_.llama_model,
                  "-c",
                  std::to_string(cfg_.ctx),
                  "-n",
                  std::to_string(cfg_.n_predict),
                  "-t",
                  std::to_string(cfg_.threads),
                  "--seed",
                  std::to_string(cfg_.seed),
                  "--temp",
                  "0.7",
                  "--top-k",
                  "20",
                  "--top-p",
                  "0.8",
                  "-no-cnv",
                  "--no-display-prompt",
                  "-p",
                  build_prompt(res.transcript)};
        o.timeout_s = cfg_.timeout_slm;
        const RunResult rr = run_process(o, stop_);
        res.slm = {rr.seconds, ram_mb(rr)};
        if (!rr.ok()) return fail(State::Generating, "SLM: " + describe_failure(rr));
        res.response = clean_response(rr.out, kMaxResponseBytes);
        if (res.response.empty()) return fail(State::Generating, "SLM: respuesta vacía");
    }

    // 4) TTS ---------------------------------------------------------------
    enter(State::Speaking);
    {
        RunOptions o;
        o.argv = {cfg_.tts_script};
        o.stdin_data = res.response;  // por entrada estándar, nunca en la línea de comandos
        o.timeout_s = cfg_.timeout_tts;
        const RunResult rr = run_process(o, stop_);
        res.tts = {rr.seconds, ram_mb(rr)};
        if (!rr.ok()) return fail(State::Speaking, "TTS: " + describe_failure(rr));
    }

    res.ok = true;
    finish();
    return res;
}

}  // namespace jarvis
