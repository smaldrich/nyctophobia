
#include "snooze.h"
#include "ui.h"
#include "ser.h"
#include "stdio.h"

snzu_Instance main_uiInstance = { 0 };
snz_Arena main_fontArena = { 0 };

void main_init(snz_Arena* scratch, SDL_Window* window) {
    SNZ_ASSERT(window || !window, "???");

    main_uiInstance = snzu_instanceInit();
    snzu_instanceSelect(&main_uiInstance);

    main_fontArena = snz_arenaInit(10000000000, "main_fontArena");
    ui_init(&main_fontArena, scratch);
}

void main_loop(float dt, snz_Arena* frameArena, snzu_Input frameInputs, HMM_Vec2 screenSize) {
    snzu_frameStart(frameArena, screenSize, dt);

    snzu_boxNew("parent");
    snzu_boxFillParent();
    snzu_boxSetColor(ui_colorBackground);

    snzu_boxNew("inner");
    snzu_boxSetSizeMarginFromParent(200);
    snzu_boxSetColor(ui_colorAccent);
    snzu_boxSetDisplayStr(&ui_labelFont, ui_colorText, "Is this shit anti-aliased?");

    HMM_Mat4 uiVP = HMM_Orthographic_RH_NO(0, screenSize.X, screenSize.Y, 0, 0.0001, 100000);
    snzu_frameDrawAndGenInteractions(frameInputs, uiVP);
}

int main() {
    snz_main("telemeter v0", NULL, main_init, main_loop);
    return EXIT_SUCCESS;
}