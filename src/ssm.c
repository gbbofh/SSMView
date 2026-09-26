#include "ssm.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { const uint8_t *bytes; size_t length, pos; } Reader;

static int take(Reader *r, size_t n, const uint8_t **out) {
    if (n > r->length - r->pos) return 0;
    *out = r->bytes + r->pos;
    r->pos += n;
    return 1;
}
static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0] | (uint16_t)p[1] << 8); }
static uint32_t u32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static float f32(const uint8_t *p) { uint32_t bits = u32(p); float f; memcpy(&f, &bits, 4); return f; }
static int string(Reader *r, char **out) {
    const uint8_t *p;
    if (!take(r, 2, &p)) return 0;
    size_t n = u16(p);
    if (!take(r, n, &p) || !(*out = malloc(n + 1))) return 0;
    memcpy(*out, p, n);
    (*out)[n] = 0;
    return 1;
}

void ssm_free(Ssm *m) {
    if (!m) return;
    if (m->animations) for (unsigned i = 0; i < m->animation_count; i++) {
        free(m->animations[i].name);
        free(m->animations[i].frame_ids);
        free(m->animations[i].durations);
    }
    if (m->texture_names) for (unsigned i = 0; i < m->texture_count; i++) free(m->texture_names[i]);
    free(m->animations);
    free(m->texture_names);
    free(m->triangles);
    free(m->frames);
    memset(m, 0, sizeof(*m));
}

int ssm_load(Ssm *m, const char *path, char *error, size_t error_size) {
    FILE *file = NULL;
    uint8_t *data = NULL;
    size_t length = 0;
    Reader r = {0};
    const uint8_t *p;
    const char *why = "invalid or truncated SSM";
    memset(m, 0, sizeof(*m));
    file = fopen(path, "rb");
    if (!file) { why = strerror(errno); goto fail; }
    if (fseek(file, 0, SEEK_END) || ftell(file) < 0) { why = "could not determine file size"; goto fail; }
    length = (size_t)ftell(file);
    if (length > 256u * 1024u * 1024u) { why = "file exceeds 256 MiB limit"; goto fail; }
    if (fseek(file, 0, SEEK_SET) || !(data = malloc(length ? length : 1)) || fread(data, 1, length, file) != length) {
        why = "could not read file"; goto fail;
    }
    fclose(file); file = NULL;
    r.bytes = data; r.length = length;
    if (!take(&r, 18, &p) || memcmp(p, "SSMO", 4) || u32(p + 4) != 1) {
        why = "expected SSMO version 1"; goto fail;
    }
    m->vertex_count = u16(p + 8);
    m->triangle_count = u16(p + 10);
    m->texture_count = u16(p + 12);
    m->frame_count = u16(p + 14);
    m->animation_count = u16(p + 16);
    if (!m->vertex_count || !m->triangle_count || !m->frame_count) {
        why = "empty geometry or frame table"; goto fail;
    }
    m->triangles = calloc(m->triangle_count, sizeof(*m->triangles));
    m->texture_names = calloc(m->texture_count ? m->texture_count : 1, sizeof(*m->texture_names));
    m->frames = calloc((size_t)m->frame_count * m->vertex_count, sizeof(*m->frames));
    m->animations = calloc(m->animation_count ? m->animation_count : 1, sizeof(*m->animations));
    if (!m->triangles || !m->texture_names || !m->frames || !m->animations) { why = "out of memory"; goto fail; }

    for (unsigned i = 0; i < m->triangle_count; i++) {
        SsmTriangle *t = &m->triangles[i];
        if (!take(&r, 36, &p)) goto fail;
        t->mesh_id = p[0];
        memcpy(t->unknown_a, p + 1, 3);
        memcpy(t->unknown_b, p + 10, 2);
        for (unsigned j = 0; j < 3; j++) {
            t->vertex[j] = u16(p + 4 + 2*j);
            if (t->vertex[j] >= m->vertex_count) { why = "triangle vertex index out of range"; goto fail; }
            for (unsigned k = 0; k < 2; k++) t->uv[j][k] = f32(p + 12 + 8*j + 4*k);
        }
    }
    m->mesh_count = m->triangles[0].mesh_id;
    if (!m->mesh_count) { why = "zero mesh count in first triangle"; goto fail; }
    for (unsigned i = 1; i < m->triangle_count; i++) if (m->triangles[i].mesh_id >= m->mesh_count) {
        why = "triangle mesh index out of range"; goto fail;
    }
    /* Two 32-bit fields between the triangle and texture tables remain unknown. */
    if (!take(&r, 8, &p)) goto fail;
    for (unsigned i = 0; i < m->texture_count; i++) {
        if (!string(&r, &m->texture_names[i]) || !take(&r, 4, &p)) goto fail;
    }
    if (!take(&r, 4, &p)) goto fail;
    for (unsigned i = 0; i < m->frame_count; i++) {
        char *frame_name = NULL;
        if (!string(&r, &frame_name)) goto fail;
        free(frame_name);
        if (!take(&r, m->mesh_count, &p)) goto fail; /* Per-mesh frame bytes, meaning unknown. */
        if (!take(&r, (size_t)m->vertex_count * 12, &p)) goto fail;
        for (unsigned j = 0; j < m->vertex_count; j++) {
            SsmVec3 *v = &m->frames[(size_t)i*m->vertex_count + j];
            v->x = f32(p + 12*j); v->y = f32(p + 12*j + 4); v->z = f32(p + 12*j + 8);
            if (!isfinite(v->x) || !isfinite(v->y) || !isfinite(v->z)) { why = "nonfinite vertex position"; goto fail; }
        }
        if (!take(&r, 8, &p)) goto fail;
    }
    for (unsigned i = 0; i < m->animation_count; i++) {
        SsmAnimation *a = &m->animations[i];
        if (!take(&r, 2, &p)) goto fail;
        a->count = u16(p);
        if (!a->count || !string(&r, &a->name)) { why = "empty or truncated animation"; goto fail; }
        a->frame_ids = calloc(a->count, sizeof(*a->frame_ids));
        a->durations = calloc(a->count, sizeof(*a->durations));
        if (!a->frame_ids || !a->durations) { why = "out of memory"; goto fail; }
        for (unsigned j = 0; j < a->count; j++) {
            if (!take(&r, 2, &p)) goto fail;
            a->frame_ids[j] = u16(p);
            if (a->frame_ids[j] >= m->frame_count) { why = "animation frame index out of range"; goto fail; }
        }
        for (unsigned j = 0; j < a->count; j++) {
            if (!take(&r, 4, &p)) goto fail;
            a->durations[j] = f32(p);
            if (!isfinite(a->durations[j]) || a->durations[j] <= 0) {
                why = "invalid animation frame duration"; goto fail;
            }
        }
        if (!take(&r, 8, &p)) goto fail;
        memcpy(a->unknown, p, 8);
    }
    m->trailing_bytes = r.length - r.pos;
    free(data);
    return 1;
fail:
    if (error && error_size) snprintf(error, error_size, "%s (offset %zu)", why, r.pos);
    if (file) fclose(file);
    free(data);
    ssm_free(m);
    return 0;
}
