#pragma once
// Mini marco de pruebas sin dependencias: cuenta fallos y devuelve código ≠ 0.
#include <cstdio>

static int g_failures = 0;

#define CHECK(cond)                                                                \
    do {                                                                           \
        if (!(cond)) {                                                             \
            std::fprintf(stderr, "FALLO %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++g_failures;                                                          \
        }                                                                          \
    } while (0)

#define CHECK_EQ(a, b)                                                                     \
    do {                                                                                   \
        if (!((a) == (b))) {                                                               \
            std::fprintf(stderr, "FALLO %s:%d: %s != %s\n", __FILE__, __LINE__, #a, #b);  \
            ++g_failures;                                                                  \
        }                                                                                  \
    } while (0)

#define TEST_RESULT()                                                \
    (g_failures == 0 ? (std::puts("OK"), 0)                          \
                     : (std::fprintf(stderr, "%d fallo(s)\n", g_failures), 1))
