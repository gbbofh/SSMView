#include <epoxy/gl.h>
#include <GLFW/glfw3.h>

#include <cglm/cglm.h>

#include <stdio.h>
#include <stdlib.h>

#include <time.h>
#include <math.h>
#include <string.h>

#include "obj.h"
#include "ssm.h"
#include "mesh.h"
#include "shader.h"

void glfw_error(int err, const char* desc)
{
    fprintf(stderr, "[glfw] error: %s\n", desc);
}

int main(int argc, char* argv[])
{
    int rv;
    GLFWwindow* wnd;

    struct ssm ssm;
    struct shader* shader;

    glfwSetErrorCallback(glfw_error);

    if(argc < 2) {

        fprintf(stderr, "error: missing required argument\n");
        exit(1);
    }

    rv = glfwInit();
    if(!rv) {

        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_RESIZABLE, false);

    wnd = glfwCreateWindow(840, 480, "Window!", NULL, NULL);
    if(!wnd) {

        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(wnd);

    shader = shader_load("test.vs.glsl", "test.fs.glsl");

    shader_set_active(shader);

    mat4 proj;
    mat4 model;
    glm_perspective_default((float)840 / (float)480, proj);

    glUniformMatrix4fv(0, 1, false, proj[0]);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    ssm_init(&ssm, argv[1]);
    if(ssm.frame_table == NULL) {

        fprintf(stderr, "error: failed to load SSM file.\n");
        exit(1);
    }

    float* vp;
    float largest_vert;
    float largest_z;
    float smallest_z;
    float largest_y;
    float largest_x;

    largest_vert = 0.0f;
    largest_z = 0.0f;
    smallest_z = 0.0f;
    largest_y = 0.0f;
    largest_x = 0.0f;

    vp = ssm.frame_table[0];
    for(int i = 0; i < ssm.num_vertices; i++) {

        if(fabs(*(vp + i)) > largest_vert) largest_vert = fabs(*(vp + i));
    }

    for(int i = 1; i < ssm.num_vertices; i += 3) {

        if(fabs(*(vp + i)) > largest_y) largest_y = fabs(*(vp + i));
        if(fabs(*(vp + i - 1)) > largest_x) largest_x = fabs(*(vp + i - 1));
        if(fabs(*(vp + i + 1)) > largest_z) largest_z = fabs(*(vp + i + 1));
        if(*(vp + i + 1) < largest_z) largest_z = *(vp + i + 1);
    }

    printf("Biggest vert: %f\n", largest_vert);
    printf("Biggest x: %f\n", largest_x);
    printf("Biggest y: %f\n", largest_y);
    printf("Biggest z: %f\n", largest_z);

    glm_mat4_identity(model);
    glm_translate_z(model, -2 * ((largest_x > largest_y) ? largest_x : largest_y));
    glm_translate_y(model, -(largest_z - smallest_z));
    glm_rotate_x(model, glm_rad(-90), model);

    printf("Loaded:  %s\n", argv[1]);
    printf("Vertices: %hu\n", ssm.num_vertices);
    printf("Triangles: %hu\n", ssm.num_indices);
    printf("Frames: %hu\n", ssm.num_frames);
    printf("Meshes: %hu\n", ssm.num_meshes);
    printf("Animations: %hu\n\n", ssm.num_anims);

    for(int i = 0; i < ssm.num_anims; i++) {

        printf("Animation: %s\n", ssm.anim_table[i].name);
    }

    int j;
    int k;
    int a;
    float delta;
    float accum;

    struct timespec cur;
    struct timespec prev;

    a = 0;
    j = 0;
    k = ssm.anim_table[a].num_frames;

    accum = 0.0f;

    clock_gettime(CLOCK_MONOTONIC, &prev);

    while(!glfwWindowShouldClose(wnd)) {

        clock_gettime(CLOCK_MONOTONIC, &cur);

        delta = (float)(cur.tv_sec - prev.tv_sec) + (float)(cur.tv_nsec - prev.tv_nsec) / 1000000000.0f;
        accum += delta;

        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glm_rotate_z(model, 0.01f, model);

        glUniformMatrix4fv(1, 1, false, model[0]);

        for(int i = 0; i < ssm.num_meshes; i++) {

            float* frame_data;

            //if(accum > 0.5f) {

            frame_data = ssm.frame_table[ssm.anim_table[a].frame_ids[j]];
            mesh_update(&ssm.meshes[i], (struct vertex*)frame_data);

            //}

            mesh_draw(&ssm.meshes[i]);
        }


        if(accum > 0.04f)  {

            j++;
            accum = 0.0f;

            if(j == k) {

                a = (a + 1) % ssm.num_anims;
                k = ssm.anim_table[a].num_frames;
                j = 0;
            }
        }

        prev = cur;

        glfwSwapBuffers(wnd);
        glfwPollEvents();
    }

    shader_unload(shader);
    ssm_term(&ssm);

    glfwDestroyWindow(wnd);
    glfwTerminate();
}

