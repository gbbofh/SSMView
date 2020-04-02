#include "shader.h"

#include "util.h"

#include <epoxy/gl.h>

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>

struct shader_internal {

    struct shader base;

    GLuint fid;
    GLuint vid;
    GLuint pid;
};

struct shader* shader_load(const char* vertex_path, const char* fragment_path)
{
    int rv;
    FILE* fp;
    char* src;
    size_t src_len;

    GLchar buf[4096];

    struct shader_internal* shader;

    shader = calloc(1, sizeof(struct shader_internal));

    /*
     * Load vertex shader
     */
    fp = fopen(vertex_path, "r");
    if(!fp) {

        fprintf(stderr, "[system] error: %s\n", strerror(errno));
        free(shader);
        shader = NULL;

        goto _shader_load_done;
    }

    fseek(fp, 0, SEEK_END);
    src_len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    src = calloc(src_len + 1, sizeof(char));
    fread(src, 1, src_len, fp);

    fclose(fp);

    shader->vid = glCreateShader(GL_VERTEX_SHADER);
    if(shader->vid == 0) {

        fprintf(stderr, "[opengl] vertex: %s\n", "Failed to create shader.");
        free(shader);
        shader = NULL;

        free(src);

        goto _shader_load_done;
    }

    glShaderSource(shader->vid, 1, (const GLchar* const *)&src, NULL);
    glCompileShader(shader->vid);
    free(src);

    glGetShaderiv(shader->vid, GL_COMPILE_STATUS, &rv);
    if(!rv) {

        glGetShaderInfoLog(shader->vid, 4096, NULL, buf);

        fprintf(stderr, "[opengl] vertex: %s\n", buf);

        free(shader);
        shader = NULL;

        goto _shader_load_done;
    }

    /*
     * Load fragment shader
     */
    fp = fopen(fragment_path, "r");
    if(!fp) {

        fprintf(stderr, "[system] error: %s\n", strerror(errno));
        free(shader);
        shader = NULL;

        goto _shader_load_done;
    }

    fseek(fp, 0, SEEK_END);
    src_len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    src = calloc(src_len + 1, sizeof(char));
    fread(src, 1, src_len, fp);

    fclose(fp);

    shader->fid = glCreateShader(GL_FRAGMENT_SHADER);
    if(shader->fid == 0) {

        fprintf(stderr, "[opengl] fragment: %s\n", "Failed to create shader.");
        free(shader);
        shader = NULL;

        free(src);

        goto _shader_load_done;
    }

    glShaderSource(shader->fid, 1, (const GLchar* const *)&src, NULL);
    glCompileShader(shader->fid);
    free(src);

    glGetShaderiv(shader->fid, GL_COMPILE_STATUS, &rv);
    if(!rv) {

        glGetShaderInfoLog(shader->fid, 4096, NULL, buf);

        fprintf(stderr, "[opengl] fragment: %s\n", buf);

        free(shader);
        shader = NULL;

        goto _shader_load_done;
    }

    shader->pid = glCreateProgram();
    if(shader->pid == 0) {

        fprintf(stderr, "[opengl] fragment: %s\n", "Failed to create program.");

        glDeleteShader(shader->vid);
        glDeleteShader(shader->fid);

        free(shader);
        shader = NULL;

        goto _shader_load_done;
    }

    glAttachShader(shader->pid, shader->vid);
    glAttachShader(shader->pid, shader->fid);

    glLinkProgram(shader->pid);
    glGetProgramiv(shader->pid, GL_LINK_STATUS, &rv);

    if(!rv) {

        glGetProgramInfoLog(shader->pid, 4096, NULL, buf);

        fprintf(stderr, "[opengl] program: %s\n", buf);

        glDeleteShader(shader->vid);
        glDeleteShader(shader->fid);

        free(shader);
        shader = NULL;

        goto _shader_load_done;
    }

    shader->base.guid = generate_guid();

_shader_load_done:
    return (struct shader*)shader;
}

void shader_unload(struct shader* shader)
{
    struct shader_internal* sh;

    sh = (struct shader_internal*)shader;

    glDeleteShader(sh->vid);
    glDeleteShader(sh->fid);
    glDeleteProgram(sh->pid);

    free(sh->base.guid);

    free(sh);
}

void shader_set_active(struct shader* shader)
{
    struct shader_internal* sh;

    if(shader == NULL) {

        glUseProgram(0);

        return;
    }

    sh = (struct shader_internal*) shader;

    glUseProgram(sh->pid);
}

