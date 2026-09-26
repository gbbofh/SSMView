#include "ssm.h"
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: ssmparse model.ssm ...\n"); return 2; }
    for (int i = 1; i < argc; i++) {
        Ssm model;
        char error[256];
        if (!ssm_load(&model, argv[i], error, sizeof(error))) {
            fprintf(stderr, "%s: %s\n", argv[i], error);
            return 1;
        }
        printf("%s: vertices=%u triangles=%u meshes=%u frames=%u animations=%u textures=%u tail=%zu\n",
            argv[i], model.vertex_count, model.triangle_count, model.mesh_count,
            model.frame_count, model.animation_count, model.texture_count, model.trailing_bytes);
        for (unsigned j = 0; j < model.animation_count; j++) {
            const SsmAnimation *a = &model.animations[j];
            printf("  %s: %u poses, first frame=%u, duration=%.3f s\n",
                a->name, a->count, a->frame_ids[0], a->durations[0]);
        }
        ssm_free(&model);
    }
    return 0;
}
