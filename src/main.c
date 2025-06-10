
#include "snooze.h"
#include "ui.h"
#include "ser.h"
#include "game.h"
#include <stdio.h>

snzu_Instance main_uiInstance = { 0 };
snz_Arena main_fontArena = { 0 };
snz_Arena main_lifetimeArena = { 0 };

snzr_FrameBuffer main_sceneFrameBuffer = { 0 };

gm_Celestial* main_rootCelestial = NULL;

void main_init(snz_Arena* scratch, SDL_Window* window) {
    SNZ_ASSERT(window || !window, "???");

    main_uiInstance = snzu_instanceInit();
    snzu_instanceSelect(&main_uiInstance);

    main_fontArena = snz_arenaInit(10000000000, "main_fontArena");
    main_lifetimeArena = snz_arenaInit(10000000000, "main_lifetimeArena");

    ui_init(&main_fontArena, scratch);

    gm_Celestial* sol = gm_celestialInit(&main_lifetimeArena, "Sol", NULL, 0, 0, 0, 1, HMM_V4(1, 1, 0, 1));
    main_rootCelestial = sol;
    //                                    name, parent, orbit radius, orbit time, orbit offset, size, color
    gm_celestialInit(&main_lifetimeArena, "Doppler", sol, 10, 45, 0, .25, HMM_V4(.7, 1, 0, 1));
    gm_Celestial* cassiopea = gm_celestialInit(&main_lifetimeArena, "Cassiopea", sol, 20, 60, .3, 1, HMM_V4(1, 1, 0, 1));
    gm_celestialInit(&main_lifetimeArena, "Cassi", cassiopea, 1.5, 10, 0, 0.25, HMM_V4(0.7, 0.7, 0.7, 1));
    gm_celestialInit(&main_lifetimeArena, "Artemis", sol, 40, 120, 4.5, 2, HMM_V4(0, .6, .7, 1));
}

void main_loop(float dt, snz_Arena* frameArena, snzu_Input og_frameInputs, HMM_Vec2 og_screenSize) {
    snzu_frameStart(frameArena, og_screenSize, dt);

    snzu_boxNew("parent");
    snzu_boxFillParent();
    snzu_boxSetColor(HMM_V4(0, 1, 0, 1));
    snzu_boxScope() {
        float leftBarWidth = 100;

        snzu_boxNew("main scene");
        snzu_boxFillParent();
        snzu_boxSetSizeFromEndAx(SNZU_AX_X, og_screenSize.X - leftBarWidth); // FIXME: size remaining fn
        {

            HMM_Vec2 size = snzu_boxGetSize();
            if (main_sceneFrameBuffer.texture.width != size.X ||
                main_sceneFrameBuffer.texture.height != size.Y) {
                snzr_frameBufferDeinit(&main_sceneFrameBuffer);
            }
            if (!main_sceneFrameBuffer.glId) {
                snzr_Texture t = snzr_textureInitRBGA(size.X, size.Y, NULL);
                main_sceneFrameBuffer = snzr_frameBufferInit(t);
            }
            snzu_boxSetTexture(main_sceneFrameBuffer.texture);
        }

        snzu_boxNew("left bar");
        snzu_boxFillParent();
        snzu_boxSetSizeFromStartAx(SNZU_AX_X, leftBarWidth);
        snzu_boxSetColor(ui_colorBackground);

        snzu_boxNew("leftBarBorder");
        snzu_boxSetSizeMarginFromParentAx(0, SNZU_AX_Y);
        snzu_boxSetStartAx(leftBarWidth - ui_thicknessUiLines, SNZU_AX_X);
        snzu_boxSetSizeFromStartAx(SNZU_AX_X, ui_thicknessUiLines);
        snzu_boxSetColor(ui_colorText);
    }

    { // game update
        float* const time = SNZU_USE_MEM(float, "time");
        *time += dt;
        gm_celestialUpdate(main_rootCelestial, *time);
    }

    {
        float* const height = SNZU_USE_MEM(float, "cameraHeight");
        *height = 60;
        // HMM_Vec2* const position = SNZU_USE_MEM(HMM_Vec2, "cameraPos");

        HMM_Vec2 fbSize = HMM_V2(main_sceneFrameBuffer.texture.width, main_sceneFrameBuffer.texture.height);
        float aspect = fbSize.X / fbSize.Y;
        float halfHeight = *height / 2;

        HMM_Mat4 cameraVP = HMM_Orthographic_RH_NO(-aspect * halfHeight, aspect * halfHeight, -halfHeight, halfHeight, 0.0001, 100000);
        // cameraVP = HMM_Mul(cameraVP, HMM_Translate(HMM_V3(-position->X, -position->Y, 1)));

        snzr_callGLFnOrError(glBindFramebuffer(GL_FRAMEBUFFER, main_sceneFrameBuffer.glId));
        snzr_callGLFnOrError(glViewport(0, 0, fbSize.X, fbSize.Y));
        snzr_callGLFnOrError(glClearColor(ui_colorBackground.X, ui_colorBackground.Y, ui_colorBackground.Z, ui_colorBackground.W));
        snzr_callGLFnOrError(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        gm_celestialDraw(main_rootCelestial, cameraVP, frameArena);
        snzr_callGLFnOrError(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        snzr_callGLFnOrError(glViewport(0, 0, og_screenSize.X, og_screenSize.Y)); // FIXME: AHHHHHHHHHHH HAVE FRAME DRAW SET VIEPORT WHY DIDN"T U DO THAT BEFORE
    }

    HMM_Mat4 uiVP = HMM_Orthographic_RH_NO(0, og_screenSize.X, og_screenSize.Y, 0, 0.0001, 100000);
    snzu_frameDrawAndGenInteractions(og_frameInputs, uiVP);
}

int main() {
    snz_main("telemeter v0", NULL, main_init, main_loop);
    return EXIT_SUCCESS;
}