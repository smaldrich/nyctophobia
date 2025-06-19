
#include "snooze.h"
#include "ui.h"
#include "ser.h"
#include "game.h"
#include <stdio.h>

snzu_Instance main_uiInstance = { 0 };
snz_Arena main_fontArena = { 0 };
snz_Arena main_lifetimeArena = { 0 };

snzr_FrameBuffer main_sceneFrameBuffer = { 0 };

gm_CelestialSlice main_celestials = { 0 };
gm_Celestial* main_rootCelestial = NULL;
gm_Celestial* main_targetCelestial = NULL;
bool main_targetCelestialZoomed = false;

ren3d_Mesh main_sphereMesh = { 0 };

void main_init(snz_Arena* scratch, SDL_Window* window) {
    SNZ_ASSERT(window || !window, "???");

    main_uiInstance = snzu_instanceInit();
    snzu_instanceSelect(&main_uiInstance);

    main_fontArena = snz_arenaInit(10000000, "main_fontArena");
    main_lifetimeArena = snz_arenaInit(10000000, "main_lifetimeArena");

    ui_init(&main_fontArena, scratch, scratch);
    ren3d_init(scratch);

    SNZ_ARENA_ARR_BEGIN(&main_lifetimeArena, gm_Celestial);
    // name, parent, orbit radius, orbit time, orbit offset, size, color
    gm_Celestial* sol = gm_celestialInit(&main_lifetimeArena, "SOL", "res/textures/sol.png", NULL, 0, 0, 0, 1, ui_colorText);
    main_rootCelestial = sol;
    gm_celestialInit(&main_lifetimeArena, "DOPPLER", "res/textures/doppler.png", sol, 10, 45, 0, .25, ui_colorText);
    gm_Celestial* cassiopea = gm_celestialInit(&main_lifetimeArena, "CASSIOPEA", "res/textures/cassiopea.png", sol, 20, 60, .3, 1, ui_colorText);
    gm_celestialInit(&main_lifetimeArena, "CASSI", "res/textures/sol.png", cassiopea, 1.5, 10, 0, 0.25, ui_colorText);
    gm_celestialInit(&main_lifetimeArena, "ARTEMIS", "res/textures/artemis.png", sol, 40, 120, 4.5, 2, ui_colorText);
    main_celestials = SNZ_ARENA_ARR_END(&main_lifetimeArena, gm_Celestial);

    main_sphereMesh = gm_sphereMeshInit(scratch, 5);
}

void main_loop(float dt, snz_Arena* frameArena, snzu_Input og_frameInputs, HMM_Vec2 og_screenSize) {
    snzu_frameStart(frameArena, og_screenSize, dt);

    snzu_boxNew("parent");
    float time = 0;
    { // game update
        float* const _time = SNZU_USE_MEM(float, "time");
        *_time += dt;
        time = *_time;
        gm_celestialUpdate(main_rootCelestial, time);
    }

    snzu_boxFillParent();
    snzu_boxScope() {
        float leftBarWidth = 100;

        _snzu_Box* sceneBox = snzu_boxNew("main scene");
        snzu_boxFillParent();
        snzu_boxSetSizeFromEndAx(SNZU_AX_X, og_screenSize.X - leftBarWidth); // FIXME: size remaining fn
        snzu_Interaction* inter = SNZU_USE_MEM(snzu_Interaction, "inter");
        snzu_boxSetInteractionOutput(inter, SNZU_IF_HOVER | SNZU_IF_MOUSE_BUTTONS | SNZU_IF_MOUSE_SCROLL);
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

            if (snzu_isNothingFocused()) {
                char inputChar = inter->keyChars[0];
                int idx = inputChar - '0';
                if (idx > 0 && idx <= main_celestials.count) { // FIXME: what happens when we have more than 10
                    SNZ_ASSERT(idx <= 9, "Keypress to select planet was past 9");
                    main_targetCelestial = &main_celestials.elems[idx - 1];
                }

                if (inter->keyAction == SNZU_ACT_DOWN) {
                    if (inter->keyCode == SDLK_ESCAPE) {
                        if (main_targetCelestialZoomed) {
                            main_targetCelestialZoomed = false;
                        } else {
                            if (main_targetCelestial) {
                                main_targetCelestial = main_targetCelestial->parent;
                            }
                            if (main_targetCelestial == main_rootCelestial) {
                                main_targetCelestial = NULL;
                            }
                        }
                    } else if (inter->keyCode == SDLK_SPACE) {
                        main_targetCelestialZoomed = !main_targetCelestialZoomed;
                    }
                } // end keydown checks
            } // end other focused check
        } // end main scene
        snzu_boxClipChildren(true);
        snzu_boxScope() {
            float* const cameraHeight = SNZU_USE_MEM(float, "cameraHeight");
            if (snzu_useMemIsPrevNew()) {
                *cameraHeight = 300;
            }
            HMM_Vec2* const cameraPosition = SNZU_USE_MEM(HMM_Vec2, "cameraPos");

            HMM_Vec2 targetPosition = HMM_V2(0, 0);
            float targetHeight = 100; // default if no celestial or root celestial is targeted
            if (main_targetCelestial) {
                targetPosition = main_targetCelestial->currentPosition;
                targetHeight = 1.5 * main_targetCelestial->orbitRadius;
            }
            *cameraHeight = HMM_Lerp(*cameraHeight, 0.2, targetHeight);
            *cameraPosition = HMM_Lerp(*cameraPosition, 0.2, targetPosition);

            float* const zoomAnim = SNZU_USE_MEM(float, "zoom anim");
            if (!main_targetCelestial) {
                main_targetCelestialZoomed = false;
            }
            snzu_easeExp(zoomAnim, main_targetCelestialZoomed, 20);
            if (main_targetCelestial) {
                *cameraHeight = HMM_Lerp(*cameraHeight, *zoomAnim, main_targetCelestial->surfaceRadius * 2 * 1.5);
            }

            HMM_Vec2 fbSize = HMM_V2(main_sceneFrameBuffer.texture.width, main_sceneFrameBuffer.texture.height);
            float aspect = fbSize.X / fbSize.Y;

            {
                float halfHeight = *cameraHeight / 2;
                HMM_Mat4 proj = HMM_Orthographic_RH_NO(-aspect * halfHeight, aspect * halfHeight, -halfHeight, halfHeight, 0, 100000);
                HMM_Mat4 cameraView = HMM_Translate(HMM_V3(-cameraPosition->X, -cameraPosition->Y, 0));

                snzr_callGLFnOrError(glBindFramebuffer(GL_FRAMEBUFFER, main_sceneFrameBuffer.glId));
                snzr_callGLFnOrError(glViewport(0, 0, fbSize.X, fbSize.Y));
                snzr_callGLFnOrError(glClearColor(ui_colorBackground.X, ui_colorBackground.Y, ui_colorBackground.Z, ui_colorBackground.W));
                snzr_callGLFnOrError(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
                glDepthMask(GL_FALSE); // so that orbit lines aren't drawn over planet render
                gm_celestialsBuild(main_celestials, sceneBox, HMM_Mul(proj, cameraView), &main_targetCelestial, *zoomAnim, frameArena);
                glDepthMask(GL_TRUE);
                if (main_targetCelestial == main_rootCelestial) {
                    main_targetCelestial = NULL;
                    main_targetCelestialZoomed = false;
                }
            }

            if (main_targetCelestialZoomed) {
                HMM_Vec2* const prev = SNZU_USE_MEM(HMM_Vec2, "prevMouse");// FIXME: build in a moue delta for drag into snooze
                HMM_Vec2 delta = { 0 };
                if (inter->dragged) {
                    delta = HMM_Sub(inter->mousePosGlobal, *prev);
                }
                *prev = inter->mousePosGlobal;

                // .X = angle in rads around the X axis
                HMM_Vec2* const cameraViewAngles = SNZU_USE_MEM(HMM_Vec2, "cameraViewAngles");
                cameraViewAngles->X -= delta.Y * 0.005;
                cameraViewAngles->Y -= delta.X * 0.005;
                cameraViewAngles->X = SNZ_MIN(cameraViewAngles->X, HMM_AngleDeg(90));
                cameraViewAngles->X = SNZ_MAX(cameraViewAngles->X, HMM_AngleDeg(-90));

                ui_debugValueF("cameraX", "%f", cameraViewAngles->X);
                ui_debugValueF("cameraY", "%f", cameraViewAngles->Y);

                // zoomed planet rendered at origin with radius = c->surfaceRadius
                float radius = main_targetCelestial->surfaceRadius;
                float halfHeight = radius * 1.5;
                HMM_Mat4 proj = HMM_Orthographic_RH_NO(-halfHeight * aspect, halfHeight * aspect, -halfHeight, halfHeight, 0, 100000);

                HMM_Vec3 cameraPosition = HMM_V3(0, 0, radius * 2);
                cameraPosition = HMM_RotateV3AxisAngle_RH(cameraPosition, HMM_V3(1, 0, 0), cameraViewAngles->X);
                cameraPosition = HMM_RotateV3AxisAngle_RH(cameraPosition, HMM_V3(0, 1, 0), cameraViewAngles->Y);
                HMM_Mat4 view = HMM_LookAt_RH(cameraPosition, HMM_V3(0, 0, 0), HMM_V3(0, 1, 0));

                HMM_Mat4 model = HMM_Scale(HMM_V3(radius, radius, radius));
                ren3d_drawMesh(&main_sphereMesh, HMM_Mul(proj, view), model);
            }
            snzr_callGLFnOrError(glBindFramebuffer(GL_FRAMEBUFFER, 0));
            snzr_callGLFnOrError(glViewport(0, 0, og_screenSize.X, og_screenSize.Y)); // FIXME: AHHHHHHHHHHH HAVE FRAME DRAW SET VIEPORT WHY DIDN"T U DO THAT BEFORE
        } // end main scene

        snzu_boxNew("left bar");
        snzu_boxFillParent();
        snzu_boxSetSizeFromStartAx(SNZU_AX_X, leftBarWidth);
        snzu_boxSetColor(ui_colorBackground);
        snzu_boxScope() {
            snzu_boxNew("line");
            snzu_boxFillParent();
            snzu_boxAlignInParent(SNZU_AX_X, SNZU_ALIGN_RIGHT);
            snzu_boxSetSizeFromEndAx(SNZU_AX_X, ui_thicknessUiLines);
            snzu_boxSetColor(ui_colorText);

            float verticalPadding = 50;

            snzu_boxNew("offset");
            snzu_boxFillParent();
            snzu_boxSetStartFromParentAx(verticalPadding, SNZU_AX_Y);
            snzu_boxSetSizePctParent(2, SNZU_AX_X);
            snzu_boxScope() {
                for (int i = 0; i < main_celestials.count; i++) {
                    gm_Celestial* c = &main_celestials.elems[i];
                    // main box acts as padding
                    snzu_boxNewF("%i left icon", i);
                    snzu_Interaction* inter = SNZU_USE_MEM(snzu_Interaction, "inter");
                    snzu_boxSetInteractionOutput(inter, SNZU_IF_MOUSE_BUTTONS | SNZU_IF_HOVER);
                    if (inter->mouseActions[SNZU_MB_LEFT] == SNZU_ACT_DOWN) { // FIXME: techinically wrong, happening late in the frame
                        main_targetCelestial = c;
                        if (c == main_rootCelestial) {
                            main_targetCelestial = NULL;
                        }
                    }

                    float* const targetedAnim = SNZU_USE_MEM(float, "targeted");
                    snzu_easeExp(targetedAnim, c == main_targetCelestial, 20);
                    float animVal = *targetedAnim + .25 * ui_hoverAnim(inter);
                    float planetSize = (c->surfaceRadius * 80) + (40 * animVal);
                    float paddedSize = planetSize + 20 + (20 * animVal);

                    snzu_boxSetSizeFromStart(HMM_V2(paddedSize, paddedSize));
                    snzu_boxScope() {
                        snzu_boxNew("planet");
                        snzu_boxSetTexture(c->texture);
                        snzu_boxSetColor(c->color);
                        snzu_boxSetSizeFromStart(HMM_V2(planetSize, planetSize));
                        snzu_boxAlignInParent(SNZU_AX_X, SNZU_ALIGN_CENTER);
                        snzu_boxAlignInParent(SNZU_AX_Y, SNZU_ALIGN_CENTER);
                        snzu_boxMoveKeepSizeRecurse(HMM_V2(planetSize * 0.75 * animVal, 0));

                        snzu_boxNew("title");
                        snzu_boxSetDisplayStr(&ui_labelFont, ui_colorText, c->name);
                        snzu_boxSetSizeFitText(10 + (20 * animVal));
                        snzu_boxAlignOuter(snzu_getSelectedBox()->prevSibling, SNZU_AX_X, SNZU_ALIGN_RIGHT);
                        snzu_boxAlignInParent(SNZU_AX_Y, SNZU_ALIGN_CENTER);
                    } // end per planet padding
                }
                snzu_boxOrderSiblingsInRowRecurse(0, SNZU_AX_Y, SNZU_ALIGN_CENTER);
                // FIXME: add a gradient fading out the planets to the left

                snzu_boxNew("menu");
                snzu_boxSetEndFromParentEnd(HMM_V2(0, -verticalPadding));
                snzu_boxSetSizeFromEnd(HMM_V2(80, 80));
                snzu_boxAlignInParent(SNZU_AX_X, SNZU_ALIGN_CENTER);
                {
                    snzu_boxSetDisplayStr(&ui_labelFont, ui_colorText, "QUIT");
                    snzu_boxSetBorder(ui_thicknessUiLines, ui_colorText);
                    snzu_Interaction* const inter = SNZU_USE_MEM(snzu_Interaction, "inter");
                    snzu_boxSetInteractionOutput(inter, SNZU_IF_MOUSE_BUTTONS | SNZU_IF_HOVER | SNZU_IF_MOUSE_SCROLL);
                    if (inter->mouseActions[SNZU_MB_LEFT] == SNZU_ACT_UP) {
                        snz_quit();
                    }

                    float* const hovered = SNZU_USE_MEM(float, "hoverAnim");
                    snzu_easeExp(hovered, inter->hovered, ui_hoverAnimSpeed);
                    snzu_boxSetColor(HMM_Lerp(ui_colorBackground, *hovered, ui_colorHoveredBackground));
                }
            } // end offset to center lists on line
        } // end left bar
    }

    ui_debugValuesBuild();
    HMM_Mat4 uiVP = HMM_Orthographic_RH_NO(0, og_screenSize.X, og_screenSize.Y, 0, 0.0001, 100000);
    snzu_frameDrawAndGenInteractions(og_frameInputs, uiVP);
}

int main() {
    snz_main("nyctophobia v0", NULL, main_init, main_loop);
    return EXIT_SUCCESS;
}
