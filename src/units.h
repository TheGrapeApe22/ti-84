#ifndef UNITS_H
#define UNITS_H
#include <stdbool.h>
#include <stddef.h>
#define UNITS_RESULT_CAPACITY 48
bool units_load(char *error, size_t capacity);
bool units_convert(const char *have, const char *want, char *result, size_t capacity);
#endif
