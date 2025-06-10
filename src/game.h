#pragma once

#include "snooze.h"
#include "ui.h"

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

    // updated vars
    HMM_Vec2 currentPosition;
};

gm_Celestial* gm_celestialInit(snz_Arena* arena, const char* name, gm_Celestial* parent, float orbitRadius, float orbitTime, float orbitStartOffset, float surfaceRadius, HMM_Vec4 color) {
    gm_Celestial* c = SNZ_ARENA_PUSH(arena, gm_Celestial);
    c->name = name;
    c->orbitRadius = orbitRadius;
    c->orbitTime = orbitTime;
    c->orbitStartOffset = orbitStartOffset;
    c->surfaceRadius = surfaceRadius;
    c->color = color;

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

void gm_orbitLineDraw(HMM_Vec2 origin, float radius, HMM_Mat4 vp, snz_Arena* scratch) {
    HMM_Vec4Slice points = SNZ_ARENA_PUSH_SLICE(scratch, 256, HMM_Vec4);
    for (int i = 0; i < points.count; i++) {
        points.elems[i].XY = HMM_RotateV2(HMM_V2(1, 0), i * (2 * HMM_PI / (points.count - 1))); // minus one to close the loop
    }
    HMM_Mat4 model = HMM_Scale(HMM_V3(radius, radius, radius));
    model = HMM_Mul(HMM_Translate(HMM_V3(origin.X, origin.Y, 0)), model);
    HMM_Mat4 mvp = HMM_Mul(vp, model);
    snzr_drawLine(points.elems, points.count, ui_colorOrbit, ui_thicknessOrbit, mvp);
}

void gm_celestialDraw(gm_Celestial* parent, HMM_Mat4 vp, snz_Arena* scratch) {
    snzr_drawRect(
        HMM_Sub(parent->currentPosition, HMM_V2(parent->surfaceRadius, parent->surfaceRadius)),
        HMM_Add(parent->currentPosition, HMM_V2(parent->surfaceRadius, parent->surfaceRadius)),
        HMM_V2(-INFINITY, -INFINITY),
        HMM_V2(INFINITY, INFINITY),
        parent->color,
        0,
        0,
        HMM_V4(0, 0, 0, 0),
        vp,
        _snzr_globs.solidTex
    );

    for (gm_Celestial* child = parent->firstChild; child; child = child->nextSibling) {
        gm_orbitLineDraw(parent->currentPosition, child->orbitRadius, vp, scratch);
        gm_celestialDraw(child, vp, scratch);
    }
}
