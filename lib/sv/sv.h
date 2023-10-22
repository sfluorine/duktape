#pragma once

#include <stdbool.h>

#define SV_FMT "%.*s"
#define SV_ARG(sv) sv.size, sv.data

typedef struct {
    const char* data;
    int size;
} sv_t;

sv_t sv_make(const char* data, int size);
sv_t sv_make_from(const char* cstr);

bool sv_equals(sv_t lhs, sv_t rhs);
