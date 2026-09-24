#include "units.h"
#include <fileioc.h>
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define DB_CAPACITY 12288
#define MAX_UNITS 128
#define MAX_PREFIXES 20
#define MAX_NAME 20
#define DIMENSIONS 7
typedef struct { char name[MAX_NAME]; double scale, offset; int8_t dim[DIMENSIONS]; } unit_t;
typedef struct { char name[MAX_NAME]; double scale; } prefix_t;
typedef struct { double scale, offset; int8_t dim[DIMENSIONS]; bool affine; } measure_t;
typedef struct { const char *at; char error[UNITS_RESULT_CAPACITY]; } parser_t;
static char db[DB_CAPACITY];
static unit_t units[MAX_UNITS];
static prefix_t prefixes[MAX_PREFIXES];
static uint8_t unit_count, prefix_count;
static bool loaded;
static void copy_text(char *out, size_t cap, const char *text) { if (cap) { strncpy(out, text, cap - 1); out[cap - 1] = '\0'; } }
static char *field(char **at) { char *value = *at, *bar; if (!value) return NULL; bar = strchr(value, '|'); if (bar) { *bar = '\0'; *at = bar + 1; } else *at = NULL; return value; }
static bool number(const char *text, double *value) { char *end; *value = strtod(text, &end); return end != text && *end == '\0'; }
static bool add_prefix(char *at) {
    char *name = field(&at), *scale = field(&at); prefix_t *item;
    if (!name || !scale || at || prefix_count == MAX_PREFIXES || strlen(name) >= MAX_NAME) return false;
    item = &prefixes[prefix_count]; if (!number(scale, &item->scale)) return false;
    copy_text(item->name, sizeof(item->name), name); prefix_count++; return true;
}
static bool add_unit(char *at) {
    char *name = field(&at), *scale = field(&at), *offset = field(&at); unit_t *item; uint8_t i;
    if (!name || !scale || !offset || unit_count == MAX_UNITS || strlen(name) >= MAX_NAME) return false;
    item = &units[unit_count]; if (!number(scale, &item->scale) || !number(offset, &item->offset)) return false;
    for (i = 0; i < DIMENSIONS; i++) { char *text = field(&at), *end; long value; if (!text) return false; value = strtol(text, &end, 10); if (end == text || *end || value < -12 || value > 12) return false; item->dim[i] = (int8_t)value; }
    if (at) return false;
    copy_text(item->name, sizeof(item->name), name);
    unit_count++;
    return true;
}
static bool read_database(char *error, size_t cap) {
    char *line = db; unsigned int line_no = 0;
    while (*line) { char *next = line, *at, *kind; bool valid = true; line_no++;
        while (*next && *next != '\r' && *next != '\n') next++;
        if (*next) { *next++ = '\0'; while (*next == '\r' || *next == '\n') next++; }
        if (*line && *line != '#') { at = line; kind = field(&at); if (!kind || kind[1]) valid = false; else if (*kind == 'P') valid = add_prefix(at); else if (*kind == 'U') valid = add_unit(at); else valid = false; if (!valid) { snprintf(error, cap, "Bad UNITDB line %u", line_no); return false; } }
        line = next;
    }
    if (!unit_count) { copy_text(error, cap, "UNITDB contains no units"); return false; } return true;
}
bool units_load(char *error, size_t cap) {
    uint8_t handle = ti_Open("UNITDB", "r"); uint16_t size; loaded = false; unit_count = 0; prefix_count = 0;
    if (!handle) { copy_text(error, cap, "Missing UNITDB AppVar"); return false; }
    size = ti_GetSize(handle); if (!size || size >= DB_CAPACITY) { ti_Close(handle); copy_text(error, cap, "UNITDB empty or too large"); return false; }
    if (ti_Read(db, 1, size, handle) != size) { ti_Close(handle); copy_text(error, cap, "Could not read UNITDB"); return false; }
    ti_Close(handle); db[size] = '\0'; if (!read_database(error, cap)) return false; loaded = true; return true;
}
static const unit_t *exact(const char *name) { uint8_t i; for (i = 0; i < unit_count; i++) if (!strcmp(name, units[i].name)) return &units[i]; return NULL; }
static bool lookup(const char *name, measure_t *out) {
    const unit_t *unit = exact(name), *best_unit = NULL; const prefix_t *best_prefix = NULL; size_t best = 0; uint8_t i;
    if (!unit) for (i = 0; i < prefix_count; i++) { size_t length = strlen(prefixes[i].name); const unit_t *candidate; if (!length || length <= best || strncmp(name, prefixes[i].name, length) || !name[length]) continue; candidate = exact(name + length); if (candidate && candidate->offset == 0.0) { best = length; best_unit = candidate; best_prefix = &prefixes[i]; } }
    if (!unit) unit = best_unit;
    if (!unit) return false;
    out->scale = unit->scale * (best_prefix ? best_prefix->scale : 1.0); out->offset = unit->offset; out->affine = unit->offset != 0.0; memcpy(out->dim, unit->dim, DIMENSIONS); return true;
}
static void fail(parser_t *p, const char *text) { if (!p->error[0]) copy_text(p->error, sizeof(p->error), text); }
static void spaces(parser_t *p) { while (isspace((unsigned char)*p->at)) p->at++; }
static bool expression(parser_t *p, measure_t *out);
static bool primary(parser_t *p, measure_t *out) {
    char name[MAX_NAME]; uint8_t length = 0; spaces(p);
    if (*p->at == '(') { p->at++; if (!expression(p, out)) return false; spaces(p); if (*p->at != ')') { fail(p, "Missing closing parenthesis"); return false; } p->at++; return true; }
    if (*p->at == '1') { p->at++; out->scale = 1; out->offset = 0; out->affine = false; memset(out->dim, 0, DIMENSIONS); return true; }
    while (isalpha((unsigned char)*p->at)) { if (length == MAX_NAME - 1) { fail(p, "Unit name is too long"); return false; } name[length++] = *p->at++; }
    name[length] = '\0'; if (!length) { fail(p, "Expected a unit name"); return false; }
    if (!lookup(name, out)) { snprintf(p->error, sizeof(p->error), "Unknown unit: %s", name); return false; } return true;
}
static bool factor(parser_t *p, measure_t *out) {
    long power = 1; uint8_t i; if (!primary(p, out)) return false; spaces(p);
    if (*p->at == '^') { char *end; p->at++; power = strtol(p->at, &end, 10); if (end == p->at || power < -12 || power > 12) { fail(p, "Invalid unit exponent"); return false; } p->at = end; }
    if (out->affine && power != 1) { fail(p, "Temperature cannot have a power"); return false; }
    if (power != 1) { out->scale = pow(out->scale, power); for (i = 0; i < DIMENSIONS; i++) out->dim[i] *= power; } return true;
}
static bool combine(parser_t *p, measure_t *left, const measure_t *right, bool divide) {
    uint8_t i; if (left->affine || right->affine) { fail(p, "Temperature must stand alone"); return false; }
    if (divide) { left->scale /= right->scale; for (i = 0; i < DIMENSIONS; i++) left->dim[i] -= right->dim[i]; }
    else { left->scale *= right->scale; for (i = 0; i < DIMENSIONS; i++) left->dim[i] += right->dim[i]; } return true;
}
static bool expression(parser_t *p, measure_t *out) {
    if (!factor(p, out)) return false;
    for (;;) { measure_t right; bool divide = false; char next; spaces(p); next = *p->at; if (next == '*' || next == '/') { divide = next == '/'; p->at++; } else if (!(isalpha((unsigned char)next) || next == '(' || next == '1')) break; if (!factor(p, &right) || !combine(p, out, &right, divide)) return false; }
    return true;
}
static bool parse(const char *text, measure_t *out, char *error, size_t cap) {
    parser_t p = { text, "" }; if (!expression(&p, out)) { copy_text(error, cap, p.error); return false; } spaces(&p); if (*p.at) { copy_text(error, cap, "Unexpected unit syntax"); return false; } return true;
}
bool units_convert(const char *have, const char *want, char *result, size_t cap) {
    char *unit_text; double value, base, answer; measure_t source, target; uint8_t i;
    if (!loaded) { copy_text(result, cap, "UNITDB is not loaded"); return false; }
    value = strtod(have, &unit_text); if (unit_text == have) { copy_text(result, cap, "You have needs a number"); return false; }
    while (isspace((unsigned char)*unit_text)) unit_text++;
    if (!*unit_text) { source.scale = 1; source.offset = 0; source.affine = false; memset(source.dim, 0, DIMENSIONS); } else if (!parse(unit_text, &source, result, cap)) return false;
    if (!parse(want, &target, result, cap)) return false;
    for (i = 0; i < DIMENSIONS; i++) if (source.dim[i] != target.dim[i]) { copy_text(result, cap, "Units are not compatible"); return false; }
    base = (value + source.offset) * source.scale; answer = base / target.scale - target.offset;
    if (fabs(answer) < 1e-12) answer = 0.0;
    snprintf(result, cap, "%.10g %s", answer, want); return true;
}
