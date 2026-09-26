#include "raylib.h"
#include "rlgl.h"
#include "ssm.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PANEL 300
#define PI_F 3.14159265358979323846f

typedef struct {
    Ssm model;
    char path[1024], message[256];
    Vector3 center, target;
    float radius, distance, yaw, pitch;
    int animation, pose, list_scroll;
    float elapsed, speed;
    bool playing, loop, wire, show_marker, mesh_visible[256];
} Viewer;

static Vector3 world(SsmVec3 v) { return (Vector3){v.x, v.z, -v.y}; }
static Vector3 lerp_vertex(const Ssm *m, int f0, int f1, unsigned v, float t) {
    SsmVec3 a = m->frames[(size_t)f0*m->vertex_count + v];
    SsmVec3 b = m->frames[(size_t)f1*m->vertex_count + v];
    return world((SsmVec3){a.x + (b.x-a.x)*t, a.y + (b.y-a.y)*t, a.z + (b.z-a.z)*t});
}
static float clampf(float x, float lo, float hi) { return fminf(hi, fmaxf(lo, x)); }

static void select_animation(Viewer *v, int index) {
    if (index < 0 || index >= v->model.animation_count) return;
    v->animation = index;
    v->pose = 0;
    v->elapsed = 0;
    v->playing = true;
}
static void reset_camera(Viewer *v) {
    v->target = v->center;
    v->distance = v->radius * 2.8f;
    v->yaw = 0.6f;
    v->pitch = 0.2f;
}
static void fit_model(Viewer *v) {
    Vector3 low = {INFINITY, INFINITY, INFINITY}, high = {-INFINITY, -INFINITY, -INFINITY};
    const Ssm *m = &v->model;
    for (size_t i = 0; i < (size_t)m->frame_count*m->vertex_count; i++) {
        Vector3 p = world(m->frames[i]);
        low.x = fminf(low.x, p.x); low.y = fminf(low.y, p.y); low.z = fminf(low.z, p.z);
        high.x = fmaxf(high.x, p.x); high.y = fmaxf(high.y, p.y); high.z = fmaxf(high.z, p.z);
    }
    v->center = (Vector3){(low.x+high.x)*0.5f, (low.y+high.y)*0.5f, (low.z+high.z)*0.5f};
    v->radius = fmaxf(0.5f, sqrtf((high.x-low.x)*(high.x-low.x) + (high.y-low.y)*(high.y-low.y) + (high.z-low.z)*(high.z-low.z))*0.5f);
    reset_camera(v);
}
static bool open_model(Viewer *v, const char *path) {
    Ssm replacement;
    char error[256];
    if (!ssm_load(&replacement, path, error, sizeof(error))) {
        snprintf(v->message, sizeof(v->message), "Load failed: %.220s", error);
        fprintf(stderr, "%s: %s\n", path, error);
        return false;
    }
    ssm_free(&v->model);
    v->model = replacement;
    snprintf(v->path, sizeof(v->path), "%s", path);
    v->message[0] = 0;
    v->animation = -1; v->pose = 0; v->elapsed = 0; v->playing = false; v->list_scroll = 0;
    for (int i = 0; i < 256; i++) v->mesh_visible[i] = true;
    fit_model(v);
    if (v->model.animation_count) select_animation(v, 0);
    printf("%s: %u vertices, %u triangle records (%u renderable), %u meshes, %u frames, %u animations, %zu unparsed tail bytes\n",
           path, v->model.vertex_count, v->model.triangle_count, v->model.triangle_count-1,
           v->model.mesh_count, v->model.frame_count, v->model.animation_count, v->model.trailing_bytes);
    return true;
}

static Camera3D camera_for(Viewer *v) {
    float h = cosf(v->pitch)*v->distance;
    Vector3 eye = {v->target.x + sinf(v->yaw)*h,
                   v->target.y + sinf(v->pitch)*v->distance,
                   v->target.z + cosf(v->yaw)*h};
    return (Camera3D){.position=eye,.target=v->target,.up={0,1,0},.fovy=50,.projection=CAMERA_PERSPECTIVE};
}
static Color mesh_color(int id) {
    static const Color colors[] = {
        {99,186,197,255}, {210,149,109,255}, {148,175,121,255},
        {181,143,196,255}, {221,189,115,255}, {113,150,206,255}
    };
    return colors[id % (int)(sizeof(colors)/sizeof(colors[0]))];
}
static void draw_model(const Viewer *v) {
    const Ssm *m = &v->model;
    if (!m->frame_count) return;
    int f0 = 0, f1 = 0;
    float blend = 0;
    if (v->animation >= 0) {
        const SsmAnimation *a = &m->animations[v->animation];
        f0 = a->frame_ids[v->pose];
        f1 = a->frame_ids[(v->pose+1 < a->count) ? v->pose+1 : (v->loop ? 0 : v->pose)];
        blend = clampf(v->elapsed/a->durations[v->pose], 0, 1);
    }
    /* Polygon winding differs across meshes; debug both sides. */
    rlDisableBackfaceCulling();
    for (unsigned i = 1; i < m->triangle_count; i++) {
        const SsmTriangle *tri = &m->triangles[i];
        if (!v->mesh_visible[tri->mesh_id]) continue;
        Vector3 a = lerp_vertex(m, f0, f1, tri->vertex[0], blend);
        Vector3 b = lerp_vertex(m, f0, f1, tri->vertex[1], blend);
        Vector3 c = lerp_vertex(m, f0, f1, tri->vertex[2], blend);
        Color base = mesh_color(tri->mesh_id);
        if (!v->wire) {
            Vector3 u = {b.x-a.x,b.y-a.y,b.z-a.z}, w = {c.x-a.x,c.y-a.y,c.z-a.z};
            Vector3 n = {u.y*w.z-u.z*w.y, u.z*w.x-u.x*w.z, u.x*w.y-u.y*w.x};
            float len = sqrtf(n.x*n.x+n.y*n.y+n.z*n.z);
            float light = len > 1e-8f ? 0.64f + 0.36f*fabsf((n.x*0.35f+n.y*0.85f+n.z*0.4f)/len) : 0.75f;
            DrawTriangle3D(a,b,c,(Color){(unsigned char)(base.r*light),(unsigned char)(base.g*light),(unsigned char)(base.b*light),255});
        } else {
            DrawLine3D(a,b,base); DrawLine3D(b,c,base); DrawLine3D(c,a,base);
        }
    }
    if (v->show_marker) {
        const SsmTriangle *tri = &m->triangles[0];
        Vector3 a = lerp_vertex(m,f0,f1,tri->vertex[0],blend);
        Vector3 b = lerp_vertex(m,f0,f1,tri->vertex[1],blend);
        Vector3 c = lerp_vertex(m,f0,f1,tri->vertex[2],blend);
        DrawLine3D(a,b,YELLOW); DrawLine3D(b,c,YELLOW); DrawLine3D(c,a,YELLOW);
        DrawSphereEx((Vector3){(a.x+b.x+c.x)/3,(a.y+b.y+c.y)/3,(a.z+b.z+c.z)/3},v->radius*0.015f,6,6,YELLOW);
    }
    rlEnableBackfaceCulling();
}

static bool button(Rectangle rect, const char *label, bool selected) {
    Vector2 mouse = GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, rect);
    DrawRectangleRec(rect, selected ? (Color){48,110,129,255} : hover ? (Color){56,66,77,255} : (Color){38,46,56,255});
    DrawRectangleLinesEx(rect,1,(Color){85,101,114,255});
    DrawText(label,(int)rect.x+9,(int)rect.y+(int)(rect.height-18)/2,18,RAYWHITE);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
static void label(int x,int y,const char *text, Color tint) { DrawText(text,x,y,17,tint); }
static void panel(Viewer *v) {
    int x = GetScreenWidth()-PANEL;
    DrawRectangle(x,0,PANEL,GetScreenHeight(),(Color){23,31,40,250});
    DrawLine(x,0,x,GetScreenHeight(),(Color){83,102,112,255});
    label(x+15,15,"SSM VIEW",(Color){124,215,225,255});
    const char *basename = strrchr(v->path,'/');
    const char *backslash = strrchr(v->path,'\\');
    if (backslash && (!basename || backslash > basename)) basename=backslash;
    basename = basename ? basename+1 : v->path;
    label(x+15,43, *basename ? basename : "Drop an .ssm file",RAYWHITE);
    if (v->model.frame_count) {
        label(x+15,70,TextFormat("%u vertices  |  %u triangles",v->model.vertex_count,v->model.triangle_count-1),LIGHTGRAY);
        label(x+15,94,TextFormat("%u meshes  |  %u poses",v->model.mesh_count,v->model.frame_count),LIGHTGRAY);
    }
    int top=132;
    if (button((Rectangle){x+14,top,92,32},v->playing ? "Pause" : "Play",v->playing)) v->playing=!v->playing;
    if (button((Rectangle){x+112,top,79,32},"Loop",v->loop)) v->loop=!v->loop;
    if (button((Rectangle){x+197,top,88,32},"Reset",false)) reset_camera(v);
    top+=40;
    if (button((Rectangle){x+14,top,130,32},"Wireframe",v->wire)) v->wire=!v->wire;
    if (button((Rectangle){x+152,top,133,32},"Marker",v->show_marker)) v->show_marker=!v->show_marker;
    top+=47;
    label(x+15,top,TextFormat("Speed: %.2fx",v->speed),LIGHTGRAY);
    if (button((Rectangle){x+177,top-5,48,30},"-",false)) v->speed=clampf(v->speed/1.25f,0.1f,4);
    if (button((Rectangle){x+232,top-5,48,30},"+",false)) v->speed=clampf(v->speed*1.25f,0.1f,4);
    top+=37;
    if (v->animation >= 0) {
        SsmAnimation *a=&v->model.animations[v->animation];
        label(x+15,top,TextFormat("Pose %d / %d  (frame %u)",v->pose+1,a->count,a->frame_ids[v->pose]),LIGHTGRAY);
    }
    top+=30;
    label(x+15,top,"Animations",(Color){124,215,225,255}); top+=28;
    int list_bottom=GetScreenHeight()-180;
    int rows=(list_bottom-top)/30;
    if (rows<0) rows=0;
    if (CheckCollisionPointRec(GetMousePosition(),(Rectangle){x,top,PANEL,(float)(list_bottom-top)})) {
        float wheel=GetMouseWheelMove();
        if (wheel) v->list_scroll+=wheel>0 ? -2 : 2;
    }
    int maxscroll=(int)v->model.animation_count-rows;
    if (maxscroll<0) maxscroll=0;
    if (v->list_scroll<0) v->list_scroll=0;
    if (v->list_scroll>maxscroll) v->list_scroll=maxscroll;
    for (int row=0;row<rows;row++) {
        int i=row+v->list_scroll;
        if (i>=v->model.animation_count) break;
        SsmAnimation *a=&v->model.animations[i];
        char title[128]; snprintf(title,sizeof(title),"%d. %.22s  (%u)",i+1,a->name,a->count);
        if (button((Rectangle){x+14,(float)(top+row*30),271,27},title,v->animation==i)) select_animation(v,i);
    }
    int bottom=GetScreenHeight()-161;
    label(x+15,bottom,"Mouse: orbit / right drag: pan",LIGHTGRAY);
    label(x+15,bottom+23,"Wheel: zoom  |  Space: play",LIGHTGRAY);
    label(x+15,bottom+46,"Left/Right: pose  |  Up/Down: clip",LIGHTGRAY);
    label(x+15,bottom+69,"W: wire  M: marker  R: reset",LIGHTGRAY);
    label(x+15,bottom+92,"Drop another .ssm to open it",LIGHTGRAY);
    if (v->message[0]) label(x+15,bottom+120,v->message,(Color){255,137,119,255});
}

int main(int argc, char **argv) {
    Viewer v={0};
    v.animation=-1; v.speed=1; v.loop=true; v.show_marker=false;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1280,780,"SSMView - raylib");
    SetTargetFPS(60);
    if (argc>1) open_model(&v,argv[1]);
    while (!WindowShouldClose()) {
        if (IsFileDropped()) {
            FilePathList files=LoadDroppedFiles();
            if (files.count) open_model(&v,files.paths[0]);
            UnloadDroppedFiles(files);
        }
        if (IsKeyPressed(KEY_SPACE)) v.playing=!v.playing;
        if (IsKeyPressed(KEY_W)) v.wire=!v.wire;
        if (IsKeyPressed(KEY_M)) v.show_marker=!v.show_marker;
        if (IsKeyPressed(KEY_R)) reset_camera(&v);
        if (v.model.animation_count) {
            if (IsKeyPressed(KEY_UP)) select_animation(&v,(v.animation+(int)v.model.animation_count-1)%v.model.animation_count);
            if (IsKeyPressed(KEY_DOWN)) select_animation(&v,(v.animation+1)%v.model.animation_count);
            if (v.animation>=0) {
                SsmAnimation *a=&v.model.animations[v.animation];
                if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
                    v.pose=(v.pose+a->count+(IsKeyPressed(KEY_LEFT)?-1:1))%a->count;
                    v.elapsed=0;v.playing=false;
                }
                if (v.playing) {
                    v.elapsed+=fminf(GetFrameTime(),0.25f)*v.speed;
                    /* Preserve leftover time and handle uneven frame durations. */
                    int steps=0;
                    while (v.elapsed>=a->durations[v.pose] && steps++ < 1024) {
                        v.elapsed-=a->durations[v.pose];
                        if (v.pose+1==a->count && !v.loop) { v.playing=false;v.elapsed=0;break; }
                        v.pose=(v.pose+1)%a->count;
                    }
                }
            }
        }
        Vector2 mouse=GetMousePosition();
        if (mouse.x<GetScreenWidth()-PANEL && v.model.frame_count) {
            Vector2 delta=GetMouseDelta();
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                v.yaw-=delta.x*0.008f; v.pitch=clampf(v.pitch+delta.y*0.008f,-1.5f,1.5f);
            }
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                float scale=v.distance*0.0015f;
                v.target.x+=(-cosf(v.yaw)*delta.x+ sinf(v.yaw)*sinf(v.pitch)*delta.y)*scale;
                v.target.y+=cosf(v.pitch)*delta.y*scale;
                v.target.z+=( sinf(v.yaw)*delta.x+ cosf(v.yaw)*sinf(v.pitch)*delta.y)*scale;
            }
            float wheel=GetMouseWheelMove();
            if (wheel) v.distance=clampf(v.distance*powf(0.86f,wheel),v.radius*0.05f,v.radius*100);
        }
        BeginDrawing();
        ClearBackground((Color){13,21,29,255});
        if (v.model.frame_count) {
            BeginMode3D(camera_for(&v));
            DrawGrid(20,v.radius*0.2f);
            float axis=v.radius*0.35f;
            DrawLine3D((Vector3){0,0,0},(Vector3){axis,0,0},RED);
            DrawLine3D((Vector3){0,0,0},(Vector3){0,axis,0},GREEN);
            DrawLine3D((Vector3){0,0,0},(Vector3){0,0,-axis},BLUE);
            draw_model(&v);
            EndMode3D();
        } else {
            DrawText("Drop an SSM file here, or pass its path as an argument.",36,48,23,LIGHTGRAY);
        }
        panel(&v);
        EndDrawing();
    }
    ssm_free(&v.model);
    CloseWindow();
    return 0;
}
