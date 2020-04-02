#include "mesh.h"

#include "util.h"

#include <epoxy/gl.h>

#include <stdlib.h>

#include <string.h>

struct mesh_internal {

    GLuint ebo;
    GLuint vbo;
    GLuint vao;
};

struct mesh* mesh_init(struct mesh* m, int nv,
                        struct vertex* vtx, int ni, int* ind)
{
    struct mesh_internal* mi;

    m->num_inds = ni;
    m->num_verts = nv;
    m->inds = calloc(ni, sizeof(int));
    m->verts = calloc(nv, sizeof(struct vertex));
    m->internal = calloc(1, sizeof(struct mesh_internal));

    memcpy(m->inds, ind, ni * sizeof(int));
    memcpy(m->verts, vtx, nv * sizeof(struct vertex));

    mi = m->internal;

    glGenBuffers(1, &mi->ebo);
    glGenBuffers(1, &mi->vbo);
    glGenVertexArrays(1, &mi->vao);

    glBindVertexArray(mi->vao);

    glBindBuffer(GL_ARRAY_BUFFER, mi->vbo);
    glBufferData(GL_ARRAY_BUFFER,
                m->num_verts * sizeof(struct vertex),
                m->verts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mi->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                m->num_inds * sizeof(int),
                m->inds, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                        sizeof(struct vertex), NULL);

    glBindVertexArray(0);

    m->guid = generate_guid();

    return m;
}

void mesh_term(struct mesh* m)
{
    struct mesh_internal* mi;

    mi = m->internal;

    glDeleteBuffers(1, &mi->ebo);
    glDeleteBuffers(1, &mi->vbo);
    glDeleteVertexArrays(1, &mi->vao);

    free(mi);
    m->internal = NULL;
    free(m->verts);
    free(m->inds);

    free(m->guid);
}

void mesh_draw(struct mesh* m)
{
    struct mesh_internal* mi;

    mi = m->internal;

    glBindVertexArray(mi->vao);
    //glDrawElements(GL_TRIANGLES, m->num_inds, GL_UNSIGNED_INT, NULL);
    //glDrawArrays(GL_POINTS, 0, m->num_verts);
    //glDrawArrays(GL_LINES, 0, m->num_verts);
    glDrawElements(GL_TRIANGLES, m->num_inds, GL_UNSIGNED_INT, NULL);
    glBindVertexArray(0);
}

void mesh_update(struct mesh* m, struct vertex* vtx)
{
    struct mesh_internal* mi;

    mi = m->internal;
    glBindVertexArray(mi->vao);

    memcpy(m->verts, vtx, m->num_verts * sizeof(struct vertex));

    glBindBuffer(GL_ARRAY_BUFFER, mi->vbo);
    glBufferData(GL_ARRAY_BUFFER,
                m->num_verts * sizeof(struct vertex),
                m->verts, GL_STATIC_DRAW);

    glBindVertexArray(0);
}

