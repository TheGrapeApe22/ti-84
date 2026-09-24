#ifndef UNITS_H
#define UNITS_H
#include <stdbool.h>
#include <stddef.h>
#define UNITS_RESULT_CAPACITY 96
bool units_load(char *error, size_t capacity);
bool units_validate_have(const char *have, char *error, size_t capacity);
bool units_convert(const char *have, const char *want, char *result, size_t capacity);
bool units_describe(const char *have, char *result, size_t capacity);
#endif
