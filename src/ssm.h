#ifndef SSM_H
#define SSM_H

#include <stddef.h>
#include <stdint.h>

typedef struct { float x, y, z; } SsmVec3;
typedef struct {
    uint8_t mesh_id;
    uint16_t vertex[3];
    float uv[3][2];
    /* Bytes 1..3 and 10..11 have not been identified. */
    uint8_t unknown_a[3], unknown_b[2];
} SsmTriangle;
typedef struct {
    char *name;
    uint16_t count;
    uint16_t *frame_ids;
    float *durations;
    uint8_t unknown[8];
} SsmAnimation;
typedef struct {
    uint16_t vertex_count, triangle_count, texture_count, frame_count, animation_count;
    uint8_t mesh_count;
    SsmTriangle *triangles; /* Record 0 is geometry too; its mesh_id byte stores mesh_count. */
    char **texture_names;
    SsmVec3 *frames; /* Contiguous [frame_count][vertex_count]. File coordinates are Z-up. */
    SsmAnimation *animations;
    size_t trailing_bytes; /* Unparsed mesh/skin data after animation table. */
} Ssm;

/* On failure, model is empty and error receives a human-readable diagnostic. */
int ssm_load(Ssm *model, const char *path, char *error, size_t error_size);
void ssm_free(Ssm *model);

#endif
