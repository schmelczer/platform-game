#include "touch.h"
#include "player.h"
#include "menu/menuFSM.h"

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif

/** \file touch.c
 * Érintőképernyős vezérlés. A játék eredetileg billentyűzetre és egérre készült, itt
 * fordítjuk le az ujjak mozgását ugyanerre: a mozgatógombok a billentyűzet állapotát
 * írják át, a szabad felületre koppintás pedig egérkattintásként viselkedik. Így a
 * játék többi része semmit nem tud arról, hogy épp érintéssel játszanak-e.
 */

#define maxFingers 10           /**< Ennél több ujjat úgyse tesz senki a képernyőre. */
#define noButton (-1)           /**< Ez az ujj nem gombon van, tehát egérként viselkedik. */

typedef enum touchButton {
    touchLeft, touchRight, touchJump, touchSlow, touchPause, touchButtonCount
} touchButton;

typedef struct touchArea {
    int x, y, r;
} touchArea;

typedef struct fingerState {
    SDL_FingerID id;
    bool isActive;
    int button;
} fingerState;

static bool touchMode = false;
static bool isLastInputTouch = false;   /**< Érintőképernyős laptopon az egér és az ujj is jó, mindig az utóbb használt számít. */
static touchArea areas[touchButtonCount];
static bool isHeld[touchButtonCount];
static bool isForced[touchButtonCount];
static fingerState fingers[maxFingers];

static const SDL_Scancode buttonKeys[touchButtonCount] = {
    SDL_SCANCODE_A, SDL_SCANCODE_D, SDL_SCANCODE_SPACE, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN
};

///Érintőképernyős eszközön futunk-e? Az első ujjlenyomás ezt amúgy is bekapcsolja.
void initTouch() {
#ifdef __EMSCRIPTEN__
    touchMode = EM_ASM_INT({
        return (navigator.maxTouchPoints > 0 || 'ontouchstart' in window) ? 1 : 0;
    });
    //érintésre képes eszközön alapból ne villanjon fel a bal felső sarokban a kurzor
    isLastInputTouch = touchMode;
#endif
}

bool isTouchMode() {
    return touchMode;
}

///Az egérrel célzáshoz kell a rajzolt kurzor, ujjal viszont csak útban lenne.
bool shouldDrawCursor() {
    return !isLastInputTouch;
}

///Egérhasználatkor visszakapcsol az egeres megjelenésre.
void noteMouseInput() {
    isLastInputTouch = false;
}

///Csak a játék közben, életben lévő játékosnál van értelme a gomboknak.
static bool areControlsVisible() {
    return touchMode && isLastInputTouch && screen != NULL && getGameState() == game && isAlive();
}

/** \brief Kiszámolja a gombok helyét a képernyő aktuális méretéhez.
 * A gombok mérete a kisebbik képernyőoldalhoz igazodik, így fekvő és álló tájolásban is
 * hüvelykujjal elérhető marad. Minden képkockában újraszámoljuk, mert az ablak bármikor
 * változhat, és így nem kell külön értesítést kötni az átméretezéshez.
 */
static void layoutButtons() {
    if(screen == NULL) return;

    int unit = smaller(screen -> w, screen -> h);
    int r = larger(28, (int)(unit * 0.1));
    int margin = (int)(unit * 0.055) + r;
    int gap = (int)(r * 0.55);

    areas[touchLeft]  = (touchArea) {margin, screen -> h - margin, r};
    areas[touchRight] = (touchArea) {margin + 2 * r + gap, screen -> h - margin, r};
    areas[touchJump]  = (touchArea) {screen -> w - margin, screen -> h - margin, r};
    areas[touchSlow]  = (touchArea) {screen -> w - margin, screen -> h - margin - 2 * r - gap, r};

    //a szünet gomb szorosan a sarokba kerül, különben álló tájolásban a pontszámra lógna
    int pauseRadius = (int)(r * 0.6);
    int pauseInset = (int)(unit * 0.035) + pauseRadius;
    areas[touchPause] = (touchArea) {screen -> w - pauseInset, pauseInset, pauseRadius};
}

///Melyik gombra esik a megadott pont? Kicsit nagyobb sugárral számolunk, mert az ujj pontatlan.
static int buttonAt(int x, int y) {
    int i;
    for(i = 0; i < touchButtonCount; i++) {
        float reach = areas[i].r * 1.2f;
        if(square(x - areas[i].x) + square(y - areas[i].y) < square(reach)) return i;
    }
    return noButton;
}

static fingerState* findFinger(SDL_FingerID id) {
    int i;
    for(i = 0; i < maxFingers; i++)
        if(fingers[i].isActive && fingers[i].id == id) return &fingers[i];
    return NULL;
}

static fingerState* addFinger(SDL_FingerID id) {
    fingerState* existing = findFinger(id);
    if(existing != NULL) return existing;

    int i;
    for(i = 0; i < maxFingers; i++)
        if(!fingers[i].isActive) {
            fingers[i] = (fingerState) {id, true, noButton};
            return &fingers[i];
        }
    return NULL;
}

///Az ujjak alapján újraszámolja, melyik gomb van épp lenyomva.
static void refreshHeldButtons() {
    int i;
    for(i = 0; i < touchButtonCount; i++) isHeld[i] = false;

    for(i = 0; i < maxFingers; i++)
        if(fingers[i].isActive && fingers[i].button != noButton)
            isHeld[fingers[i].button] = true;
}

/** \brief Az ujjakhoz tartozó eventeket kezeli.
 * A gombokra eső ujjak a gombjukhoz tapadnak, a többi ujj egérként viselkedik: mozgatja a
 * kurzort és kattint is vele. Emiatt lehet egyszerre menni, ugrani és lőni, amit egyetlen
 * emulált egérrel nem lehetne.
 * \param a feldolgozandó event
 * \param a bal egérgomb állapota, ezt a hívó adja át a játéknak
 * \param prematureClick a beállítás menü miatt kell
 */
void handleFingerEvent(SDL_Event* event, bool* isLMBDown, bool* prematureClick) {
    if(screen == NULL) return;

    touchMode = isLastInputTouch = true;
    layoutButtons();

    int x = (int)(event -> tfinger.x * screen -> w);
    int y = (int)(event -> tfinger.y * screen -> h);
    putInsideInterval(0, &x, screen -> w - 1);
    putInsideInterval(0, &y, screen -> h - 1);

    fingerState* finger;

    switch(event -> type) {
        case SDL_FINGERDOWN:
            finger = addFinger(event -> tfinger.fingerId);
            if(finger == NULL) break;

            finger -> button = areControlsVisible() ? buttonAt(x, y) : noButton;

            if(finger -> button == noButton) {
                setMousePosition(x, y);
                *isLMBDown = true;
                handleClicks(prematureClick);
            }
            //a szünet gomb kattintásként viselkedik, nem tartásként
            else if(finger -> button == touchPause) whatToDoOnEscape();
            break;

        case SDL_FINGERMOTION:
            finger = findFinger(event -> tfinger.fingerId);
            if(finger == NULL) break;

            if(finger -> button == noButton) setMousePosition(x, y);
            //a két iránygomb közt át lehet csúsztatni az ujjat, a többi gomb megtartja az ujját
            else if(finger -> button == touchLeft || finger -> button == touchRight) {
                int moved = buttonAt(x, y);
                if(moved == touchLeft || moved == touchRight) finger -> button = moved;
            }
            break;

        case SDL_FINGERUP:
            finger = findFinger(event -> tfinger.fingerId);
            if(finger == NULL) break;

            if(finger -> button == noButton) *isLMBDown = *prematureClick = false;
            finger -> isActive = false;
            break;
    }

    refreshHeldButtons();
}

bool isTouchSlowMo() {
    return touchMode && isHeld[touchSlow];
}

/** \brief A megnyomott iránygombokat billentyűlenyomásnak állítja be.
 * Csak azokat a billentyűket írja át, amiket ő maga nyomott le, így egy érintőképernyős
 * laptopon a valódi billentyűzet is használható marad.
 */
void applyTouchKeys() {
    if(!touchMode || keys == NULL) return;

    int i;
    for(i = 0; i < touchButtonCount; i++) {
        if(buttonKeys[i] == SDL_SCANCODE_UNKNOWN) continue;

        if(isHeld[i]) {
            keys[buttonKeys[i]] = 1;
            isForced[i] = true;
        }
        else if(isForced[i]) {
            keys[buttonKeys[i]] = 0;
            isForced[i] = false;
        }
    }
}

///Egy gomb korongja, lenyomva világosabb.
static void drawButtonBase(touchArea area, bool pressed) {
    filledCircleColor(screen, area.x, area.y, area.r, pressed ? 0xFFFFFF44 : 0xFFFFFF1E);
    aacircleColor(screen, area.x, area.y, area.r, pressed ? 0xFFFFFFCC : 0xFFFFFF66);
}

///Háromszög alakú nyíl, az irány -1 (balra), +1 (jobbra) vagy 0 (felfelé).
static void drawArrow(touchArea area, int direction) {
    float size = area.r * 0.45f;
    float vx[3], vy[3];

    if(direction == 0) {
        vx[0] = area.x;         vy[0] = area.y - size;
        vx[1] = area.x - size;  vy[1] = area.y + size * 0.8;
        vx[2] = area.x + size;  vy[2] = area.y + size * 0.8;
    }
    else {
        vx[0] = area.x + direction * size;          vy[0] = area.y;
        vx[1] = area.x - direction * size * 0.8;    vy[1] = area.y - size;
        vx[2] = area.x - direction * size * 0.8;    vy[2] = area.y + size;
    }

    filledPolygonRGBA(screen, vx, vy, 3, 255, 255, 255, 220);
    aapolygonRGBA(screen, vx, vy, 3, 255, 255, 255, 220);
}

///Az időlassítás ikonja egy óra, a szünet gombé két álló csík.
static void drawSlowMoIcon(touchArea area) {
    Sint16 size = (Sint16)(area.r * 0.45);
    aacircleColor(screen, area.x, area.y, size, 0xFFFFFFDC);
    lineColor(screen, area.x, area.y, area.x, area.y - (Sint16)(size * 0.7), 0xFFFFFFDC);
    lineColor(screen, area.x, area.y, area.x + (Sint16)(size * 0.5), area.y, 0xFFFFFFDC);
}

static void drawPauseIcon(touchArea area) {
    int width = larger(2, (int)(area.r * 0.16));
    int height = (int)(area.r * 0.45);
    int gap = (int)(area.r * 0.2);

    boxRGBA(screen, area.x - gap - width, area.y - height, area.x - gap, area.y + height, 255, 255, 255, 220);
    boxRGBA(screen, area.x + gap, area.y - height, area.x + gap + width, area.y + height, 255, 255, 255, 220);
}

///Kirajzolja a képernyőre az érintéses gombokat, egérrel játszva semmit nem csinál.
void drawTouchControls() {
    if(!areControlsVisible()) return;

    layoutButtons();

    int i;
    for(i = 0; i < touchButtonCount; i++) drawButtonBase(areas[i], isHeld[i]);

    drawArrow(areas[touchLeft], -1);
    drawArrow(areas[touchRight], 1);
    drawArrow(areas[touchJump], 0);
    drawSlowMoIcon(areas[touchSlow]);
    drawPauseIcon(areas[touchPause]);
}


#ifdef __EMSCRIPTEN__

/** \brief Megjeleníti a névbeíráshoz tartozó html mezőt.
 * Az emscripten SDL2-je nem hozza elő a mobilos billentyűzetet, ráadásul iOS-en csak a
 * felhasználó koppintására lehet fókuszt adni egy mezőnek. Ezért egy látható beviteli
 * mezőt teszünk a vászon fölé, amit a játékos maga koppint meg.
 */
EM_JS(void, showNameOverlay, (), {
    var box = document.getElementById('nameOverlay');

    if (!box) {
        box = document.createElement('form');
        box.id = 'nameOverlay';
        box.style.cssText =
            'position:fixed;left:50%;top:62%;transform:translate(-50%,-50%);z-index:10;' +
            'display:flex;gap:8px;padding:10px;border-radius:12px;' +
            'background:rgba(26,5,5,0.92);border:1px solid rgba(232,213,213,0.35);' +
            'font:16px system-ui,sans-serif;touch-action:manipulation;';

        var field = document.createElement('input');
        field.type = 'text';
        field.maxLength = 40;
        field.placeholder = 'Neved';
        field.autocomplete = 'off';
        field.autocapitalize = 'off';
        field.style.cssText =
            'width:9em;min-width:0;padding:10px 12px;font:inherit;color:#e8d5d5;' +
            'background:rgba(255,255,255,0.08);border:0;border-radius:8px;outline:none;';

        var done = document.createElement('button');
        done.type = 'submit';
        done.textContent = 'Kész';
        done.style.cssText =
            'padding:10px 16px;font:inherit;color:#1a0505;background:#e8d5d5;' +
            'border:0;border-radius:8px;';

        box.appendChild(field);
        box.appendChild(done);
        document.body.appendChild(box);

        field.addEventListener('input', function () {
            Module.ccall('mobileNameChanged', null, ['string'], [field.value]);
        });

        box.addEventListener('submit', function (event) {
            event.preventDefault();
            field.blur();
            Module.ccall('mobileNameSubmitted', null, [], []);
        });
    }

    var current = box.querySelector('input');
    current.value = "";
    box.style.display = 'flex';
});

EM_JS(void, hideNameOverlay, (), {
    var box = document.getElementById('nameOverlay');
    if (box) {
        box.querySelector('input').blur();
        box.style.display = 'none';
    }
});

///A html mezőbe írt teljes nevet veszi át.
EMSCRIPTEN_KEEPALIVE void mobileNameChanged(const char* utf8) {
    setNameInput(utf8);
}

///A "Kész" gomb megnyomását továbbítja.
EMSCRIPTEN_KEEPALIVE void mobileNameSubmitted() {
    submitNameInput();
}

#endif

///A névbeíró állapotba lépve előveszi, kilépve elrejti a böngésző beviteli mezőjét.
void syncSoftKeyboard(states state) {
#ifdef __EMSCRIPTEN__
    static bool isShown = false;
    bool isWanted = touchMode && state == nameInput;

    if(isWanted == isShown) return;
    isShown = isWanted;

    if(isWanted) showNameOverlay();
    else hideNameOverlay();
#else
    (void) state;
#endif
}
