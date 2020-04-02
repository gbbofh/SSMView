#include "ssm.h"

#include "obj.h"
#include "mesh.h"

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>

#include <cglm/cglm.h>

void ssm_init(struct ssm* ssm, const char* path)
{
    FILE* fp;

    char* data;
    size_t offset;
    size_t length;

    short string_len;

    data = NULL;
    offset = 0;
    length = 0;

    memset(ssm, 0, sizeof(struct ssm));

    fp = fopen(path, "r");
    if(fp == NULL) {

        fprintf(stderr, "error: %s\n", strerror(errno));
        goto ssmi_done;
    }

    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    data = malloc(length);
    fread(data, 1, length, fp);
    fclose(fp);

    if(strncmp(data, "SSMO", 4) != 0) {

        fprintf(stderr, "error: %s\n", "not a valid SSM file."); 
        goto ssmi_done;
    }

    offset = 8;

    ssm->num_vertices   = *(unsigned short*)(data + offset);
    ssm->num_indices    = *(unsigned short*)(data + offset + 2);
    ssm->num_textures   = *(unsigned short*)(data + offset + 4);
    ssm->num_frames     = *(unsigned short*)(data + offset + 6);
    ssm->num_anims      = *(unsigned short*)(data + offset + 8);

    offset += 10;

    ssm->num_meshes     = *(data + offset);
    *(data + offset)    = 0; /* number of meshes is encoded in first index */

    ssm->index_table = calloc(ssm->num_indices, sizeof(struct ssm_index));
    ssm->anim_table = calloc(ssm->num_anims, sizeof(struct ssm_anim));
    ssm->frame_table = calloc(ssm->num_frames, sizeof(float**));
    ssm->meshes = calloc(ssm->num_meshes, sizeof(struct mesh));

    for(int i = 0; i < ssm->num_indices; i++) {

        ssm->index_table[i].mesh_id = *(data + offset);
        ssm->index_table[i].p0 = *(unsigned short*)(data + offset + 4);
        ssm->index_table[i].p1 = *(unsigned short*)(data + offset + 6);
        ssm->index_table[i].p2 = *(unsigned short*)(data + offset + 8);

        offset += 36;
    }

    offset += 8;

    for(int i = 0; i < ssm->num_textures; i++) {

        string_len = *(unsigned short*)(data + offset);

        offset += 2;
        offset += string_len + 4;
    }

    offset += 4;

    for(int i = 0; i < ssm->num_frames; i++) {

        ssm->frame_table[i] = calloc(3 * ssm->num_vertices, sizeof(float));

        string_len = *(unsigned short*)(data + offset);

        offset += 2;
        offset += string_len + ssm->num_meshes;

        memcpy(ssm->frame_table[i], data + offset, 12 *ssm->num_vertices);
        offset += 12 * ssm->num_vertices + 8;
    }

    for(int i = 0; i < ssm->num_anims; i++) {

        short s;

        ssm->anim_table[i].num_frames = *(unsigned short*)(data + offset);
        string_len = *(short*)(data + offset + 2);

        ssm->anim_table[i].name = calloc(string_len, sizeof(char));
        strncpy(ssm->anim_table[i].name, data + offset + 4, string_len);

        offset += string_len + 4;

        ssm->anim_table[i].frame_ids = calloc(ssm->anim_table[i].num_frames, sizeof(int));
        for(int j = 0; j < ssm->anim_table[i].num_frames; j++) {

            s = *(unsigned short*)(data + offset);
            ssm->anim_table[i].frame_ids[j] = s;

            offset += 2;
        }

        /* The usage of the data here is not known */
        offset += 4 * ssm->anim_table[i].num_frames + 8;
    }

    for(int i = 0, j = 0; i < ssm->num_meshes; i++) {

        int j0;
        int* inds;

        j0 = j;

        while(ssm->index_table[j].mesh_id == i) j++;

        inds = calloc(3 * (j - j0), sizeof(int));
        for(int k = 0; k < (j - j0); k++) {

            inds[3 * k + 0] = ssm->index_table[k + j0].p0;
            inds[3 * k + 1] = ssm->index_table[k + j0].p1;
            inds[3 * k + 2] = ssm->index_table[k + j0].p2;
        }

        mesh_init(&ssm->meshes[i], ssm->num_vertices,
                (struct vertex*)ssm->frame_table[0], 3 * (j - j0), inds);

        free(inds);
    }

ssmi_done:
    free(data);
}

void ssm_term(struct ssm* ssm)
{

    for(int i = 0; i < ssm->num_frames; i++) {

        free(ssm->frame_table[i]);
    }

    for(int i = 0; i < ssm->num_anims; i++) {

        free(ssm->anim_table[i].frame_ids);
    }

    free(ssm->anim_table);
    free(ssm->frame_table);
    free(ssm->index_table);

    for(int i = 0; i < ssm->num_meshes; i++) {

        mesh_term(&ssm->meshes[i]);
    }
}

