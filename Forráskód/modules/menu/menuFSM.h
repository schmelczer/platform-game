#ifndef MENUFSM_H_INCLUDED
#define MENUFSM_H_INCLUDED

    #include "../global.h"
    #include "menuDraw.h"
    #include "../world.h"
    #include "../saves.h"
    #include "../auxiliary/auxSDL.h"
    #include "../auxiliary/unicodePrint.h"

    #define fadeTime 1750   /**< Halál esetén meddig tartson a sötétítés? */
    #define slowMoMult 0.3  /**< A slowMo (jobb egérgomb), mennyiszeresére lassítsa az időt? */

    states getGameState();

    void handleClicks(bool* prematureClick);
    void doStuff(bool isLMBDown, bool isRMBDown, bool prematureClick);
    void whatToDoOnEscape();
    void processTyping(SDL_Event event);
    void processTextInput(const char* utf8);

#endif // MENUFSM_H_INCLUDED
