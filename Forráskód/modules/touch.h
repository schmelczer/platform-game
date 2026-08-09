#ifndef TOUCH_H_INCLUDED
#define TOUCH_H_INCLUDED

    #include "global.h"
    #include "menu/menuDraw.h"

    void initTouch();
    bool isTouchMode();
    bool shouldDrawCursor();
    void noteMouseInput();
    void handleFingerEvent(SDL_Event* event, bool* isLMBDown, bool* prematureClick);
    bool isTouchSlowMo();
    void applyTouchKeys();
    void drawTouchControls();
    void syncSoftKeyboard(states state);

#endif // TOUCH_H_INCLUDED
