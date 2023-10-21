#include "sv.h"

sv_t sv_make(const char* data, int size) {
    return (sv_t) {
        .data = data,
        .size = size,
    };
}

sv_t sv_make_from(const char* cstr) {
    int size = 0;
    while (cstr[size]) {
        size++;
    }

    return (sv_t) {
        .data = cstr,
        .size = size,
    };
}

bool sv_equals(sv_t lhs, sv_t rhs) {
    if (lhs.size != rhs.size) {
        return false;
    }

    for (int i = 0; i < lhs.size; i++) {
        if (lhs.data[i] != rhs.data[i]) {
            return false;
        }
    }

    return true;
}
