#pragma once

#define LOCATION_FMT "(%d:%d)"
#define LOCATION_ARG(arg) arg.line, arg.col

typedef struct {
    int line;
    int col;
} location_t;

location_t location_make(int, int);
