#ifndef SHADER_H
#define SHADER_H

struct shader {

    char* guid;
};

struct shader* shader_load(const char* vertex_path, const char* fragment_path);
void shader_unload(struct shader* shader);

void shader_set_active(struct shader* shader);

#endif

