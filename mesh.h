#ifndef MESH_H
#define MESH_H

struct vertex {

    float position[3];
    //float normal[3];
};

struct mesh {

    char* guid;

    int* inds;
    struct vertex* verts;

    void* internal;

    int num_verts;
    int num_inds;
};

struct mesh* mesh_init(struct mesh* m, int nv,
                        struct vertex* vtx, int ni, int* ind);

void mesh_term(struct mesh* m);

void mesh_draw(struct mesh* m);

void mesh_update(struct mesh* m, struct vertex* vtx);

#endif

