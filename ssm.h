#ifndef SSM_H
#define SSM_H

struct obj;
struct mesh;

struct ssm_anim;
struct ssm_frame;
struct ssm_index;

struct ssm_anim {

    int num_frames;
    int* frame_ids;

    char* name;
};

struct ssm_index {

    char mesh_id;

    int p0;
    int p1;
    int p2;
};

struct ssm {

    int num_anims;
    int num_meshes;
    int num_frames;
    int num_indices;
    int num_vertices;
    int num_textures;

    float** frame_table;
    struct ssm_anim* anim_table;
    struct ssm_index* index_table;

    struct mesh* meshes;
};

void ssm_init(struct ssm* ssm, const char* path);
void ssm_term(struct ssm* ssm);

void ssm_load(struct obj* ssm, const char* path);
void ssm_unload(struct obj* ssm);

#endif

