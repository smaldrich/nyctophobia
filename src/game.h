#pragma once

#include "snooze.h"
#include "ui.h"

// out and scratch may be same arena
void gm_spherePointsGen(snz_Arena* scratch, snz_Arena* out, int subdivs) {
    float phi = (sqrtf(5) + 1) / 2.0f;
    HMM_Vec3 initialPoints[20] = {
        {-1, phi, 0}, {1, phi, 0}, {-1, -phi, 0}, {1, -phi, 0},
        {-1, phi, 0}, {1, phi, 0}, {-1, -phi, 0}, {1, -phi, 0},
        {0, -1, phi}, {0, 1, phi}, {0, -1, -phi}, {0, 1, -phi},
        {phi, 0, -1}, {phi, 0, 1}, {-phi, 0, -1}, {-phi, 0, 1},
    };
    int32_t initialTris[] = {
        0,11,5,0,5,1,0,1,7,0,7,10,0,10,11,
        1,5,9,5,11,4,11,10,2,10,7,6,7,1,8,
        3,9,4,3,4,2,3,2,6,3,6,8,3,8,9,
        4,9,5,2,4,11,6,2,10,8,6,7,9,8,1,
    };

    int32_tSlice indicies = {
        .elems = initialTris,
        .count = sizeof(initialTris) / sizeof(*initialTris),
    };
    HMM_Vec3Slice verts = {
        .elems = initialPoints,
        .count = sizeof(initialPoints) / sizeof(*initialPoints),
    };
    for (int subdivisionIdx = 0; subdivisionIdx < subdivs; subdivisionIdx++) {
        snz_Arena* arena = (subdivisionIdx == subdivs - 1) ? out : scratch;
        SNZ_ARENA_ARR_BEGIN(arena, HMM_Vec3);
        for (int triangleIdx = 0; triangleIdx < indicies.count / 3; triangleIdx++) {
            HMM_Vec3* newPts = SNZ_ARENA_PUSH_ARR(arena, 6, HMM_Vec3);
            int triStartIdx = triangleIdx * 3;
            newPts[0] = verts.elems[indicies.elems[triStartIdx + 0]];
            newPts[1] = verts.elems[indicies.elems[triStartIdx + 1]];
            newPts[2] = verts.elems[indicies.elems[triStartIdx + 2]];
            newPts[3] = HMM_DivV3F(HMM_Add(newPts[0], newPts[1]), 2.0f);
            newPts[4] = HMM_DivV3F(HMM_Add(newPts[1], newPts[2]), 2.0f);
            newPts[5] = HMM_DivV3F(HMM_Add(newPts[2], newPts[0]), 2.0f);

            if (subdivisionIdx == subdivs - 1) {
                for (int ptIdx = 0; ptIdx < 6; ptIdx++) {
                    newPts[ptIdx] = HMM_Norm(newPts[ptIdx]);
                }
            } // end conditionally normalizing points
        }
        HMM_Vec3Slice newVerts = SNZ_ARENA_ARR_END(arena, HMM_Vec3);

        SNZ_ARENA_ARR_BEGIN(arena, int32_t);
        for (int triangleIdx = 0; triangleIdx < indicies.count / 3; triangleIdx++) {
            int startIdx = 6 * triangleIdx; // six verts per subdivided triangle
            int* indexes = SNZ_ARENA_PUSH_ARR(arena, 12, int); // 3 indexes for each of 4 triangles

            indexes[0] = startIdx + 0;
            indexes[1] = startIdx + 3;
            indexes[2] = startIdx + 5;

            indexes[3] = startIdx + 3;
            indexes[4] = startIdx + 1;
            indexes[5] = startIdx + 4;

            indexes[6] = startIdx + 4;
            indexes[7] = startIdx + 2;
            indexes[8] = startIdx + 5;

            indexes[9] = startIdx + 3;
            indexes[10] = startIdx + 4;
            indexes[11] = startIdx + 5;
        }
        int32_tSlice newIndicies = SNZ_ARENA_ARR_END(arena, int32_t);

        verts = newVerts;
        indicies = newIndicies;
    }
}

typedef struct gm_Celestial gm_Celestial;
struct gm_Celestial {
    // constant vars
    const char* name;
    gm_Celestial* parent;
    gm_Celestial* firstChild;
    gm_Celestial* nextSibling;
    float orbitRadius;
    float orbitTime;
    float surfaceRadius;
    float orbitStartOffset; // in rads, represents initial angle along orbit at t = 0
    HMM_Vec4 color;
    snzr_Texture texture;

    // updated vars
    HMM_Vec2 currentPosition;
};

SNZ_SLICE(gm_Celestial);

gm_Celestial* gm_celestialInit(snz_Arena* arena, const char* name, const char* texturePath, gm_Celestial* parent, float orbitRadius, float orbitTime, float orbitStartOffset, float surfaceRadius, HMM_Vec4 color) {
    gm_Celestial* c = SNZ_ARENA_PUSH(arena, gm_Celestial);
    c->name = name;
    c->orbitRadius = orbitRadius;
    c->orbitTime = orbitTime;
    c->orbitStartOffset = orbitStartOffset;
    c->surfaceRadius = surfaceRadius;
    c->color = color;
    c->texture = ui_texFromFile(texturePath);

    c->parent = parent;
    if (c->parent) {
        gm_Celestial** ptrToNextPtr = &c->parent->firstChild;
        while (*ptrToNextPtr) { // FIXME: cutoff
            ptrToNextPtr = &(*ptrToNextPtr)->nextSibling;
        }
        *ptrToNextPtr = c;
    }
    return c;
}

// expects parent to be updated, updates children, body should be non-null
// if no parent, body position updated to 0, 0
void gm_celestialUpdate(gm_Celestial* body, float time) {
    if (!body->parent) {
        body->currentPosition = HMM_V2(0, 0);
    } else {
        float currentAngle = body->orbitStartOffset + HMM_AngleTurn(time / body->orbitTime);
        body->currentPosition = HMM_RotateV2(HMM_V2(body->orbitRadius, 0), currentAngle);
        body->currentPosition = HMM_Add(body->currentPosition, body->parent->currentPosition);
    }

    for (gm_Celestial* child = body->firstChild; child; child = child->nextSibling) {
        gm_celestialUpdate(child, time);
    }
}

void gm_orbitLineDraw(HMM_Vec2 fadeOrigin, HMM_Vec2 origin, float radius, HMM_Mat4 vp, snz_Arena* scratch) {
    HMM_Vec4Slice points = SNZ_ARENA_PUSH_SLICE(scratch, 256, HMM_Vec4);
    for (int i = 0; i < points.count; i++) {
        float angle = i * (2 * HMM_PI / (points.count - 1));  // minus one to close the loop
        points.elems[i].XY = HMM_RotateV2(HMM_V2(radius, 0), angle);
        points.elems[i].XY = HMM_Add(points.elems[i].XY, origin);
    }
    snzr_drawLineFaded(
        points.elems, points.count,
        ui_colorOrbit, ui_thicknessOrbit,
        vp,
        HMM_V3(fadeOrigin.X, fadeOrigin.Y, 0), 0, radius * 1.8);
}

// expects GL ctx to be on a framebuffer
// expects a valid snzu_Instance also
void gm_celestialsBuild(gm_CelestialSlice celestials, _snzu_Box* parentBox, HMM_Mat4 vp, gm_Celestial** outTargetCelestial, snz_Arena* scratch) {
    HMM_Vec2 parentStart = parentBox->start;
    HMM_Vec2 parentSize = snzu_boxGetSizePtr(parentBox);

    HMM_Mat4 transform = HMM_Scale(HMM_V3(1, -1, 1)); // flip y axis to go down positive
    transform = HMM_Mul(HMM_Translate(HMM_V3(1, 1, 0)), transform); // -1 to 1 -> 0 to 2
    transform = HMM_Mul(HMM_Scale(HMM_V3(0.5 * parentSize.X, 0.5 * parentSize.Y, 1)), transform); // 0 to 2 -> 0 to parentSize
    transform = HMM_Mul(HMM_Translate(HMM_V3(parentStart.X, parentStart.Y, 0)), transform); // 0 to parentSize -> parentStart to parentEnd
    transform = HMM_Mul(transform, vp);

    for (int i = 0; i < celestials.count; i++) {
        gm_Celestial* c = &celestials.elems[i];
        snzu_boxNewF("%d", i);
        snzu_boxSetTexture(c->texture);
        snzu_boxSetColor(c->color);

        snzu_Interaction* inter = SNZU_USE_MEM(snzu_Interaction, "inter");
        snzu_boxSetInteractionOutput(inter, SNZU_IF_MOUSE_BUTTONS | SNZU_IF_HOVER);
        if (inter->mouseActions[SNZU_MB_LEFT] == SNZU_ACT_DOWN) {
            *outTargetCelestial = c; // FIXME: wrong spot in the frame bro
        }

        {
            float hoverAnim = ui_hoverAnim(inter);
            float radius = c->surfaceRadius * (1 + 0.25 * hoverAnim);

            HMM_Vec4 position4 = { .W = 1 };
            position4.XY = HMM_Add(c->currentPosition, HMM_V2(-radius, radius)); // upper left is screen space min,min
            HMM_Vec4 pt = HMM_Mul(transform, position4);
            snzu_boxSetStart(pt.XY);

            position4 = (HMM_Vec4){ .W = 1 };
            position4.XY = HMM_Add(c->currentPosition, HMM_V2(radius, -radius)); // bottom right is screen space max,max
            pt = HMM_Mul(transform, position4);
            snzu_boxSetEnd(pt.XY);
        }

        for (gm_Celestial* child = c->firstChild; child; child = child->nextSibling) {
            gm_orbitLineDraw(child->currentPosition, c->currentPosition, child->orbitRadius, vp, scratch);
        }
    }
}
