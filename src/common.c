#include <common.h>

location_t location_make(int line, int col) {
    return (location_t) {
        .line = line,
        .col = col,
    };
}
