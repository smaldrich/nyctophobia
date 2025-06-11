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

SNZ_SLICE(gm_Celestial);

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
        float angle = i * (2 * HMM_PI / (points.count - 1));  // minus one to close the loop
        points.elems[i].XY = HMM_RotateV2(HMM_V2(1, 0), angle);
    }
    HMM_Mat4 model = HMM_Scale(HMM_V3(radius, radius, radius));
    model = HMM_Mul(HMM_Translate(HMM_V3(origin.X, origin.Y, 0)), model);
    HMM_Mat4 mvp = HMM_Mul(vp, model);
    snzr_drawLine(points.elems, points.count, ui_colorOrbit, ui_thicknessOrbit, mvp);
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
            gm_orbitLineDraw(c->currentPosition, child->orbitRadius, vp, scratch);
        }
    }
}
