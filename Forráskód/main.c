#include <time.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif

#include "modules/global.h"
#include "modules/camera.h"
#include "modules/color.h"
#include "modules/floaters.h"
#include "modules/particles.h"
#include "modules/world.h"
#include "modules/map.h"
#include "modules/menu/menuDraw.h"
#include "modules/menu/menuFSM.h"
#include "modules/touch.h"


/**\mainpage Leírás
 * \section Röviden
 * Egy könnyed, egyszerűen megismerhető, mégis jutalmazó játékélményt nyújtó programot szerettem volna.
 * \section Hosszabban
 * A játéktér 3D-s kockákból álló, dinamikus méretű, és véletlenszerűen generált.
 * A játékosnak a generált akadályok tetején található jutalmakat kell gyűjtenie.
 * Ezt természetesen repülő ellenségek nehezítik, akik meg is keresik a játékost.
 * Nem csak az ellenségeket, hanem a pályát is lehet rombolni a karakter által lőtt lövedékekkel.
 * A játékost segíti még az időlassítás lehetősége is. Persze egyik sincs ingyen, mindkét képesség
 * használata a pontszám csökkenésével jár. A pontszám nem kerülhet nulla alá.
 * A játék aktuális állása fájlba menthető és onnan be is tölthető.
 * Ezen felül a program egy toplistát is tartalmaz a legjobb eredményekről.
 * \section Érdekességek
 * - Nem triviális a térbeli megjelenítés, a pálya generálás illetve a mentés tömörítésének eljárása.
 * Ezeket bővebben kifejtve a megfelelő modulok függvényeinél lehet megtalálni. Röviden viszont itt is megmagyarázom őket.
 * A 3D-s hatást egy kamera fizikai modelljével érem el. Általában nem ez a leggyorsabb megoldás, viszont a játék egyszerű grafikájából
 * kifolyólag (kizárólag téglatestek vannak benne) itt nagyon jól beválik. Persze rengeteg optimalizálás kell hozzá, hogy hardveres gyorsítás nélkül,
 * kizárólag a processzoron futva is élvezhetően gyors legyen. Ilyen optimalizálások a nem egyesével történő rajzolás; ha hasonló tulajdonságú kockák vannak
 * egymás alatt, akkor azokat egy magas oszlopként rajzolja. Csak azokat az oldalakat rajzolja, amik valóban látszanak. Illetve értelemszerűen,
 * csak azokat a kockákat rajzolja, amelyikek látszanak.
 * Ezen felül a rajzolás sorrendje is igencsak érdekes kihívást tár elénk, de erről már csak a camera.c modulban kerül szó.
 * - A pályagenerálás nem túl bonyolult, két részből áll. Előszőr a földszintet építi, ahol random magasságú oszlopok vannak,
 * csak az van megszabva, hogy mekkora lehet két szomszédos oszlop közt a legnagyobb magasságkülönbség. A felső szint már érdekesebb. Ott vannak a platformok.
 * A pályát x irányban kisebb egységekre osztjuk, minden egységben egy "oszlopot" építünk. Az oszlop alatt olyan platformokat értünk, amik közt a távolság olyan, hogy
 * a játék aktuális fizikájával mindig legalább egy irányban tovább lehet haladni róluk. Igazából ennyi az egész.
 * - A tömörítés a három közül a legprimitívebb megoldás, viszont nincs szükségünk bonyolultabbra, ez is elégséges. Mivel a pályák nagyrésze igen repetitív, ezért
 * a blockok is gyakran ismétlődnek, ezt az ismétlődést a: "(ismétlődés száma)block típusa" formában rögzítjük. Erről bővebben a save.c modulban. Fontos megjegyezni, hogy
 * a játék nincsen felkészítve a fájljaiba való külső beavatkozásra, tehát ezt igyekezzünk elkerülni.
 * \section Modulok
 * - camera.c: minden, ami 3d-s megjelenítéssel kapcsolatos
 * - map.c: minden, ami a pálya generálásával és ütközésteszteléssel kapcsolatos
 * - floaters.c: ellenségek, lövedékek
 * - particles.c: hóeséssel kapcsolatos függvények és tömbök
 * - player.c: nagyrészt a player mozgásáért felelős
 * - saves.c: mentés, betöltés, mentés fájlok kezelése
 * - world.c: meghívja a világ létrehozásához szükséges függvényeket
 * - color.c: a pálya színei itt készülnek (HSL -> RGB átalakítás is van itt)
 * - menuFSM.c: a játékban és a menüben történő inputra reagál elsősorban, itt zajlanak az állapotátmenetek
 * - menuDraw.c: a menüt rajzolja le, illetve meghívja a játéktér rajzolását is, ha szükséges
 * - unicodePrint.c: a név sugallja, az unicode szövegek kezelése és megjelenítése a feladata
 * - auxSDL.c: a megfelelő sdl-es függvények meghívása, illetve az egér interakciók kezelése
 * - auxFunctions.c: gyakran használ inline függvények
 * \section Kompatibilitás
 * A program lefordításához szükséges az SDL2 és az SDL2_ttf könyvtár, a rajzoló primitíveket a program magával hozza.
 * Natívan és Emscriptennel webre fordítva is működik. A játék fix FPS-sel fut, ez jelenleg 50-re van beállítva, így átlagos teljesítményű gépeken is
 * el kell futnia, viszont ehhez szükség van egy átlagos képességű gépre, szóval ez egy lényeges rendszerkövetelmény.
 * A játék működéséhez szükségesek a megfelelő ttf-fájlok.
 * \section Készítette
 * Schmelczer András, 2017. 12. 03.
 */


/**
 * \file main.c
 */


bool isLMBDown = false, isRMBDown = false, prematureClick;

static Uint32 backgroundColor;
static Uint32 previousTicks;
static double accumulator = 0;
static bool quitRequested = false;

static void handleEvents() {
    SDL_Event event;

    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_QUIT:

                quitRequested = true;
                break;


            case SDL_MOUSEBUTTONDOWN:

                noteMouseInput();
                if(event.button.button == SDL_BUTTON_LEFT) handleClicks(&prematureClick);
                if(event.button.button == SDL_BUTTON_RIGHT) isRMBDown = true;
                if(event.button.button == SDL_BUTTON_LEFT) isLMBDown = true;
                break;

            case SDL_MOUSEBUTTONUP:

                if(event.button.button == SDL_BUTTON_RIGHT) isRMBDown = false;
                if(event.button.button == SDL_BUTTON_LEFT) isLMBDown = prematureClick = false;
                break;

            case SDL_MOUSEMOTION:

                noteMouseInput();
                setMousePosition(event.motion.x, event.motion.y);
                break;

            case SDL_KEYDOWN:

                if(event.key.keysym.sym == SDLK_ESCAPE) whatToDoOnEscape();
                else processTyping(event);
                break;

            case SDL_TEXTINPUT:

                processTextInput(event.text.text);
                break;

            case SDL_FINGERDOWN:
            case SDL_FINGERMOTION:
            case SDL_FINGERUP:

                handleFingerEvent(&event, &isLMBDown, &prematureClick);
                break;

        }
    }
}

static void frame() {
    const double step = 1000.0 / FPS;

    handleEvents();
    handleResize();

    Uint32 now = SDL_GetTicks();
    double elapsed = now - previousTicks;
    previousTicks = now;
    if(elapsed > 5 * step) elapsed = 5 * step;
    accumulator += elapsed;

    bool hasDrawn = false;
    while(accumulator >= step) {
        accumulator -= step;

        bool isLastStep = accumulator < step;
        setRenderingEnabled(isLastStep);

        if(isLastStep) SDL_FillRect(screen, NULL, backgroundColor);
        applyTouchKeys();
        doStuff(isLMBDown, isRMBDown || isTouchSlowMo(), prematureClick);
        hasDrawn = hasDrawn || isLastStep;
    }
    setRenderingEnabled(true);

    syncSoftKeyboard(getGameState());

    if(hasDrawn) presentFrame();
#ifdef __EMSCRIPTEN__
    if(getGameState() == quit || quitRequested) emscripten_cancel_main_loop();
#else
    if(!hasDrawn) SDL_Delay(1);
#endif
}

int main(int argc, char *argv[]) {

    srand(time(NULL));

    //az érintéseket magunk dolgozzuk fel, az sdl egérré alakítása egyszerre csak egy ujjat tudna
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");

    openWindow();
    initTouch();
    keys = (unsigned char*) SDL_GetKeyboardState(NULL);
    SDL_StartTextInput();

    backgroundColor = SDL_MapRGB(screen -> format, 40, 0, 0);

    createMenuWorld();
    previousTicks = SDL_GetTicks();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(frame, 0, 1);
#else
    while(getGameState() != quit && !quitRequested) frame();

    //Felszabadítás
    freeFloaterList();
    freeColorMap();
    freeMap();
    freeRenderMap();
    freeParticleList();
    SDL_FreeSurface(text);
    freeFonts();
    closeWindow();
    SDL_Quit();
#endif

    return 0;
}
