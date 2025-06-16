#pragma once

#include "GLAD/include/glad/glad.h"
#include "HMM/HandmadeMath.h"
#include "snooze.h"

// FIXME: move to snz?
static char* _ren3d_loadFileToStr(const char* path, snz_Arena* scratch) {
    FILE* f = fopen(path, "rb");
    SNZ_ASSERTF(f != NULL, "Opening file failed. Path: %s", path);

    fseek(f, 0L, SEEK_END);
    uint64_t size = ftell(f);
    fseek(f, 0L, SEEK_SET);

    char* chars = SNZ_ARENA_PUSH_ARR(scratch, size, char);
    fread(chars, sizeof(char), size, f);
    fclose(f);
    return chars;
}

typedef struct {
    HMM_Vec3 pos;
    HMM_Vec4 color;
} ren3d_Vert;

SNZ_SLICE(ren3d_Vert);

typedef struct {
    uint64_t vertCount;
    uint32_t vaId;
    uint32_t vertexBufferId;

    uint32_t indexBufferId;
    uint32_t indexCount;
} ren3d_Mesh;

// verts are not retained CPU side, and may be removed immediately following this call
ren3d_Mesh ren3d_meshInit(ren3d_Vert* verts, uint64_t vertCount, uint32_t* indicies, uint64_t indexCount) {
    ren3d_Mesh out = {
        .vaId = 0,
        .vertexBufferId = 0,
        .vertCount = vertCount,
    };
    // FIXME: safe GL calls here :)
    glGenVertexArrays(1, &out.vaId);
    glBindVertexArray(out.vaId);

    glGenBuffers(1, &out.vertexBufferId);
    glBindBuffer(GL_ARRAY_BUFFER, out.vertexBufferId);
    uint64_t vertSize = sizeof(ren3d_Vert);
    glBufferData(GL_ARRAY_BUFFER, vertCount * vertSize, verts, GL_STATIC_DRAW);

    glGenBuffers(1, &out.indexBufferId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out.indexBufferId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(uint32_t), indicies, GL_STATIC_DRAW);
    out.indexCount = indexCount;

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertSize, NULL);  // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, vertSize, (void*)offsetof(ren3d_Vert, color));  // color
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    return out;
}

void ren3d_meshDeinit(ren3d_Mesh* mesh) {
    glDeleteVertexArrays(1, &mesh->vaId);
    glDeleteBuffers(1, &mesh->vertexBufferId);
    memset(mesh, 0, sizeof(*mesh));
}

static uint32_t _ren3d_flatId;

void ren3d_init(snz_Arena* scratch) {
    const char* vertSrc = _ren3d_loadFileToStr("res/shaders/flat.vert", scratch);
    const char* fragSrc = _ren3d_loadFileToStr("res/shaders/flat.frag", scratch);
    _ren3d_flatId = snzr_shaderInit(vertSrc, fragSrc, scratch);
}

void ren3d_drawMesh(const ren3d_Mesh* mesh, HMM_Mat4 vp, HMM_Mat4 model) {
    snzr_callGLFnOrError(glUseProgram(_ren3d_flatId));

    // FIXME: gl safe uniform loc calls
    int loc = glGetUniformLocation(_ren3d_flatId, "uVP");
    snzr_callGLFnOrError(glUniformMatrix4fv(loc, 1, false, (float*)&vp));

    loc = glGetUniformLocation(_ren3d_flatId, "uModel");
    snzr_callGLFnOrError(glUniformMatrix4fv(loc, 1, false, (float*)&model));

    snzr_callGLFnOrError(glBindVertexArray(mesh->vaId));
    snzr_callGLFnOrError(glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexBufferId));
    snzr_callGLFnOrError(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->indexBufferId));
    // snzr_callGLFnOrError(glDrawArrays(GL_TRIANGLES, 0, mesh->indexCount / 5));
    snzr_callGLFnOrError(glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, NULL));
}
