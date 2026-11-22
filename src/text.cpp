#include "jarvis/text.hpp"

namespace jarvis {

namespace {

const char* const kSystemPrompt =
    "Eres Jarvis-son, un asistente de voz. Responde siempre en español "
    "latinoamericano, en una o dos frases cortas, sin listas ni formato.";

bool is_control_or_space(unsigned char c) { return c < 0x20 || c == 0x7f || c == ' '; }

void erase_all(std::string& s, const std::string& pattern) {
    std::size_t pos;
    while ((pos = s.find(pattern)) != std::string::npos) s.erase(pos, pattern.size());
}

// Borra todo lo que esté entre open y close (incluidos). Si open no se cierra,
// solo se borra el delimitador de apertura.
void erase_delimited(std::string& s, const std::string& open, const std::string& close) {
    std::size_t pos = 0;
    while ((pos = s.find(open, pos)) != std::string::npos) {
        std::size_t end = s.find(close, pos + open.size());
        if (end == std::string::npos) {
            s.erase(pos, open.size());
        } else {
            s.erase(pos, end + close.size() - pos);
        }
    }
}

void erase_special_tokens(std::string& s) {
    erase_delimited(s, "<|", "|>");
    erase_all(s, "<think>");
    erase_all(s, "</think>");
}

// Hay contenido si queda al menos una letra o dígito. Los signos de
// puntuación de dos y tres bytes (por ejemplo ¿ ¡ « » — …, cuyo primer byte
// es 0xC2 o 0xE2) no cuentan; las letras acentuadas y la ñ (0xC3) sí.
bool has_content(const std::string& s) {
    for (std::size_t i = 0; i < s.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) return true;
        if (c == 0xC2 || c == 0xE2) {
            while (i + 1 < s.size() && (static_cast<unsigned char>(s[i + 1]) & 0xC0) == 0x80) ++i;
            continue;
        }
        if (c >= 0x80 && (c & 0xC0) != 0x80) return true;  // otra letra no ASCII
    }
    return false;
}

}  // namespace

std::string normalize_spaces(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    bool pending_space = false;
    for (unsigned char c : in) {
        if (is_control_or_space(c)) {
            pending_space = !out.empty();
        } else {
            if (pending_space) {
                out.push_back(' ');
                pending_space = false;
            }
            out.push_back(static_cast<char>(c));
        }
    }
    return out;
}

std::string truncate_utf8(const std::string& s, std::size_t max_bytes) {
    if (s.size() <= max_bytes) return s;
    std::size_t cut = max_bytes;
    while (cut > 0 && (static_cast<unsigned char>(s[cut]) & 0xC0) == 0x80) --cut;
    std::string out = s.substr(0, cut);
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

std::string clean_transcript(const std::string& raw, std::size_t max_bytes) {
    std::string s = raw;
    erase_special_tokens(s);
    erase_delimited(s, "[", "]");
    erase_delimited(s, "(", ")");
    erase_all(s, "\xE2\x99\xAA");  // ♪
    s = truncate_utf8(normalize_spaces(s), max_bytes);
    return has_content(s) ? s : std::string();
}

std::string clean_response(const std::string& raw, std::size_t max_bytes) {
    std::string s = raw;
    erase_all(s, "[end of text]");
    erase_special_tokens(s);
    s = truncate_utf8(normalize_spaces(s), max_bytes);
    return has_content(s) ? s : std::string();
}

std::string build_prompt(const std::string& transcript) {
    std::string p;
    p += "<|im_start|>system\n";
    p += kSystemPrompt;
    p += "<|im_end|>\n<|im_start|>user\n";
    p += transcript;
    p += "<|im_end|>\n<|im_start|>assistant\n<think>\n\n</think>\n\n";
    return p;
}

}  // namespace jarvis
