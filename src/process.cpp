#include "jarvis/process.hpp"

#include <fcntl.h>
#include <poll.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>

namespace jarvis {

namespace {

using Clock = std::chrono::steady_clock;

void close_fd(int& fd) {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void set_cloexec(int fd) {
    int flags = fcntl(fd, F_GETFD, 0);
    if (flags >= 0) fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
}

void ignore_sigpipe() {
    struct sigaction sa;
    std::memset(&sa, 0, sizeof sa);
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPIPE, &sa, nullptr);
}

void append_capped(std::string& out, const char* data, std::size_t n, std::size_t cap) {
    if (out.size() >= cap) return;
    out.append(data, std::min(n, cap - out.size()));
}

}  // namespace

RunResult run_process(const RunOptions& opt, const volatile std::sig_atomic_t* stop_flag) {
    RunResult r;
    if (opt.argv.empty()) {
        r.error = "comando vacío";
        return r;
    }
    ignore_sigpipe();

    // Se prepara argv antes del fork: en el hijo no se reserva memoria.
    std::vector<char*> args;
    args.reserve(opt.argv.size() + 1);
    for (const std::string& a : opt.argv) args.push_back(const_cast<char*>(a.c_str()));
    args.push_back(nullptr);

    int in_pipe[2] = {-1, -1};
    int out_pipe[2] = {-1, -1};
    if (pipe(in_pipe) != 0) {
        r.error = "no se pudo crear la tubería de entrada";
        return r;
    }
    if (pipe(out_pipe) != 0) {
        close_fd(in_pipe[0]);
        close_fd(in_pipe[1]);
        r.error = "no se pudo crear la tubería de salida";
        return r;
    }
    set_cloexec(in_pipe[0]);
    set_cloexec(in_pipe[1]);
    set_cloexec(out_pipe[0]);
    set_cloexec(out_pipe[1]);

    const Clock::time_point start = Clock::now();
    const pid_t pid = fork();
    if (pid < 0) {
        close_fd(in_pipe[0]);
        close_fd(in_pipe[1]);
        close_fd(out_pipe[0]);
        close_fd(out_pipe[1]);
        r.error = "fork falló";
        return r;
    }

    if (pid == 0) {
        // Hijo: grupo propio para poder matar también a sus descendientes.
        setpgid(0, 0);
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) dup2(devnull, STDERR_FILENO);
        execvp(args[0], args.data());
        _exit(127);
    }

    setpgid(pid, pid);  // también desde el padre, para evitar la carrera
    close_fd(in_pipe[0]);
    close_fd(out_pipe[1]);
    set_nonblocking(in_pipe[1]);
    set_nonblocking(out_pipe[0]);
    r.started = true;

    std::size_t written = 0;
    bool in_open = true;
    if (opt.stdin_data.empty()) {
        close_fd(in_pipe[1]);
        in_open = false;
    }
    bool out_open = true;
    bool reaped = false;
    bool killed = false;
    int status = 0;
    struct rusage usage;
    std::memset(&usage, 0, sizeof usage);
    const Clock::time_point deadline = start + std::chrono::seconds(opt.timeout_s);
    char buf[4096];

    auto kill_group = [&]() {
        ::kill(-pid, SIGKILL);
        ::kill(pid, SIGKILL);
        killed = true;
    };

    while (true) {
        if (!killed) {
            if (stop_flag != nullptr && *stop_flag != 0) {
                r.interrupted = true;
                kill_group();
            } else if (Clock::now() > deadline) {
                r.timed_out = true;
                kill_group();
            }
        }

        struct pollfd fds[2];
        int nfds = 0;
        int idx_in = -1;
        int idx_out = -1;
        if (in_open) {
            fds[nfds] = {in_pipe[1], POLLOUT, 0};
            idx_in = nfds++;
        }
        if (out_open) {
            fds[nfds] = {out_pipe[0], POLLIN, 0};
            idx_out = nfds++;
        }
        const int pr = poll(fds, nfds, 50);  // con nfds == 0 solo espera 50 ms
        if (pr < 0 && errno != EINTR) break;

        if (pr > 0) {
            if (idx_in >= 0 && (fds[idx_in].revents & (POLLOUT | POLLERR | POLLHUP))) {
                const ssize_t w = write(in_pipe[1], opt.stdin_data.data() + written,
                                        opt.stdin_data.size() - written);
                if (w > 0) {
                    written += static_cast<std::size_t>(w);
                    if (written >= opt.stdin_data.size()) {
                        close_fd(in_pipe[1]);
                        in_open = false;
                    }
                } else if (w < 0 && errno != EAGAIN && errno != EINTR) {
                    close_fd(in_pipe[1]);
                    in_open = false;
                }
            }
            if (idx_out >= 0 && (fds[idx_out].revents & (POLLIN | POLLERR | POLLHUP))) {
                const ssize_t n = read(out_pipe[0], buf, sizeof buf);
                if (n > 0) {
                    append_capped(r.out, buf, static_cast<std::size_t>(n), opt.max_output);
                } else if (n == 0 || (errno != EAGAIN && errno != EINTR)) {
                    close_fd(out_pipe[0]);
                    out_open = false;
                }
            }
        }

        if (!reaped) {
            const pid_t w = wait4(pid, &status, WNOHANG, &usage);
            if (w == pid) reaped = true;
        }
        if (reaped) break;
    }

    if (!reaped) {  // salida anómala del bucle: no dejar zombis
        kill_group();
        wait4(pid, &status, 0, &usage);
    }

    // Lo que quedara en la tubería de salida.
    if (out_open) {
        ssize_t n;
        while ((n = read(out_pipe[0], buf, sizeof buf)) > 0)
            append_capped(r.out, buf, static_cast<std::size_t>(n), opt.max_output);
    }
    close_fd(in_pipe[1]);
    close_fd(out_pipe[0]);

    // Si el proceso terminó dejando descendientes en su grupo, se eliminan:
    // un turno no debe dejar procesos huérfanos.
    ::kill(-pid, SIGKILL);

    r.seconds = std::chrono::duration<double>(Clock::now() - start).count();
    if (WIFEXITED(status)) {
        r.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        r.term_signal = WTERMSIG(status);
    }
    r.max_rss_kb = usage.ru_maxrss;  // en KB en Linux y Android
    return r;
}

std::string describe_failure(const RunResult& r) {
    if (!r.started) return r.error.empty() ? "no se pudo lanzar el proceso" : r.error;
    if (r.interrupted) return "interrumpido por el usuario";
    if (r.timed_out) return "tiempo agotado";
    if (r.term_signal == SIGKILL)
        return "el proceso fue terminado por la señal 9 (posible falta de memoria)";
    if (r.term_signal != 0)
        return "el proceso terminó por la señal " + std::to_string(r.term_signal);
    if (r.exit_code == 127)
        return "código de salida 127 (no se encontró o no se pudo ejecutar el programa)";
    return "código de salida " + std::to_string(r.exit_code);
}

}  // namespace jarvis
