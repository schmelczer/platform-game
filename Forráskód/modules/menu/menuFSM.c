#include "menuFSM.h"


int currentOptions[] = {2000, 2000, 3, 450, 6000};
int currentSpawnRate, fadeStartTime, elapsedTime = 0, nameIndex = 0, cursorBlinking = 0;

bool isDollying, saved = false;
wchar_t name[100] = {0};
FILE* topList;

states gameState = mainMenu;        /**< Kezdőállapot. */

///Visszaadja a jelenlegi állapotot, a quit-hez kell a mainnek.
states getGameState() {
    return gameState;
}

///Minden állapothoz tartozó kattintós eventet kezel, a beállítások menü miatt kell a prematureClick, hogy a belépést ne érzékelje beállításmódosításnak.
void handleClicks(bool* prematureClick) {
    switch(gameState) {
        case game:
            if(isAlive()) shoot(getMousePosition());
            break;

        case mainMenu:
            if(isMouseOverRect(50, menuStart, 15, menuTav / 2)) {
                gameState = initGame;
            }
            else if(isMouseOverRect(50, menuStart + menuTav, 15, menuTav / 2)) {
                gameState = loadMenu;
            }
            else if(isMouseOverRect(50, menuStart + menuTav*2, 15, menuTav / 2)) {
                gameState = optionsMenu;
                *prematureClick = true;
            }
            else if(isMouseOverRect(50, menuStart + menuTav*3, 15, menuTav / 2)) {
                loadTopList();
                gameState = topListMenu;
            }
            else if(isMouseOverRect(50, menuStart + menuTav*4, 15, menuTav / 2))
                gameState = infoMenu;
            break;
        case pauseMenu:
            if(isMouseOverRect(50, menuStart - 5, 15, menuTav/2)) {
                gameState = game;
                updateText(getPoints());
            }
            if(isMouseOverRect(50, menuStart - 5 + menuTav, 15, menuTav/2)) {
                saveGame(currentOptions);
                saved = true;
            }
            if(isMouseOverRect(50, menuStart - 5 + menuTav*2, 15, menuTav/2)) {
                createMenuWorld();
                gameState = loadMenu;
            }
            if(isMouseOverRect(50, menuStart - 5 + menuTav*3, 15, menuTav/2)) {
                createMenuWorld();
                gameState = mainMenu;
            }
            break;

        case loadMenu:
            if(isMouseOverRect(50, menuStart + menuTav * 5, 15, menuTav/2)) {
                gameState = mainMenu;
                break;
            }

            int i, j = 0;
            for(i = 0; i < maxNOSaves; i++) {
                if(existingSaves[i]) {
                    if(isMouseOverRect(50, loadStart + loadTav*j, 25, 3)) {
                        loadGame(currentOptions, i);
                        gameState = initLoadedGame;
                    }
                    else if(isMouseOverRect(50, loadStart + loadTav*j + 5, 25, 2)) {
                        deleteSave(i);
                    }
                    j++;
                }
            }

            break;
        case topListMenu:
        case optionsMenu:
        case infoMenu:
            if(isMouseOverRect(50, menuStart + menuTav*5, 15, menuTav/2))
                gameState = mainMenu;
            break;
    }
}

/** \brief Ez igenis egy kifejező függyvény név. Az állapothoz megfelelő műveletek elvéűgzése, és a kimenet létrehozása. Tehát ha a játék magától csinál valamit, azt itt csinálja.
 * Ez az egész egy nagy állapotgép. Igazából a mainben is lehetne, de szerintem így szebben néz ki.
 * \param egérgomb bal
 * \param egérgomb jobb
 * \param prematureClick a beállítás menünek
 * A deltaTime a menüben kisebb, mint máshol, így egyszerűbben meg leeht oldani a hóesés lassítását. Amúgy az egész játék deltaTimera épült, ha
 * c-ben megbízhatóan lehetne miliseces pontossággal lehetne időt mérni, akkor az FPS sokkal dinamikusabb lehetne (a jelenlegi fix 50-hez képest).
 * Ilyet viszont nem lehet, de azért benne hagytam, egyrészről a menü miatt, másrészről az időlassítás miatt, meg esetleg azért is, mert később, más
 * környezetben még lehet jól jön ez a megoldás.
 */
void doStuff(bool isLMBDown, bool isRMBDown, bool prematureClick) {
    bool isInSlowMo = false;
    int deltaTime = 20;
    switch(gameState) {
        case mainMenu:
            drawMainMenu(deltaTime);
            break;

        case optionsMenu:
            drawOptionsMenu(deltaTime, currentOptions, isLMBDown && !prematureClick);
            break;

        case infoMenu:
            drawInfoMenu(deltaTime);
            break;

        case topListMenu:
            drawTopList(deltaTime);
            break;

        case pauseMenu:
            deltaTime = 0;
            drawMap(deltaTime);
            drawPauseMenu(saved);
            break;

        case loadMenu:
            drawLoadMenu(deltaTime);
            break;

        case initGame:
            createWorld(currentOptions);
        case initLoadedGame:
            currentSpawnRate = currentOptions[4];
            isDollying = true;
            elapsedTime = 0;
            cameraDollyZ(0, true);
            gameState = game;
            break;

        case game:
            isInSlowMo = slowTime(isRMBDown);
            deltaTime = (int)(1000.0 / FPS) * (isInSlowMo ? slowMoMult : 1);
            elapsedTime += deltaTime;
            if(isDollying) isDollying = cameraDollyZ(deltaTime, false);

            drawMap(deltaTime);

            if(isAlive()) {
                movePlayer(keys, deltaTime);
                moveFloaters(deltaTime);
                drawPointsText();

                if(elapsedTime > currentSpawnRate) {
                    createEnemy();
                    elapsedTime = 0;
                    if(currentSpawnRate > 1500) currentSpawnRate -= 10;
                }
            }
            else {
                fadeStartTime = elapsedTime;
                name[0] = 0;
                nameIndex = 0;
                gameState = fading;
            }
            break;
        case fading:
            drawMap(0);

            elapsedTime += deltaTime;
            if((elapsedTime - fadeStartTime) / fadeTime >= 1)
                gameState = nameInput;
            boxRGBA(screen, 0, 0, screen -> w, screen -> h, 0, 0, 0, smaller(255, (int)(255 * (elapsedTime - fadeStartTime) / fadeTime)));
            break;

        case nameInput:
            boxRGBA(screen, 0, 0, screen -> w, screen -> h, 0, 0, 0, 255);
            cursorBlinking += deltaTime;
            if(cursorBlinking > 1000) cursorBlinking = 0;
            char typedName[400], loseText[600];
            wideToUtf8(name, typedName, sizeof(typedName));
            snprintf(loseText, sizeof(loseText), "%s%s vesztett, a pontszáma %d.", typedName, cursorBlinking > 500 ? "|" : "", getPoints());
            utf8PrintCentered(loseText, 50, 50, small);
            break;
    }
    if(isAlive()) drawMouse(isInSlowMo);
}

///A nameInputból returnnel és ESC-vel is ki lehet lépni, ezért kell ez a függvény rá, amit mindkét esetben meg lehet hívni.
static void escapeFromNameInput() {
    if(gameState == nameInput) {
        gameState = mainMenu;
        setDie(false);
        createMenuWorld();
        topList = fopen("rang.lst", "a");
        if(topList != NULL) {
            char utf8[400];
            wideToUtf8(name[0] == 0 ? L"Névtelen" : name, utf8, sizeof(utf8));
            fprintf(topList, "%s,%d\n", utf8, getPoints());
            fclose(topList);
        }
    }
}

///Az ESC billenytű lenyomását kezeli.
void whatToDoOnEscape() {
    switch(gameState) {
        case pauseMenu:
            gameState = game;
            updateText(getPoints());
            break;
        case game:
            if(isAlive()) {
                gameState = pauseMenu;
                saved = false;
            }
            break;
        case fading:
            gameState = nameInput;
            break;
        case nameInput:
            escapeFromNameInput();
            gameState = mainMenu;
            break;
        default:
            gameState = mainMenu;
    }
}

///A szöveges bemenetet továbbítja egy wchar_t-os stringbe.
void processTyping(SDL_Event event) {
    if(gameState  == nameInput) {
        SDL_Keycode key = event.key.keysym.sym;
        if(key == SDLK_BACKSPACE) { if(nameIndex > 0) name[--nameIndex] = 0; }
        else if(key == SDLK_RETURN || key == SDLK_KP_ENTER) escapeFromNameInput();
        name[nameIndex] = 0;
    }
}

///A beírt karaktereket utf-8-ból átalakítva fűzi a névhez.
void processTextInput(const char* utf8) {
    if(gameState == nameInput) {
        wchar_t typed[16];
        int count = utf8ToWide(utf8, typed, sizeof(typed) / sizeof(wchar_t));

        int i;
        for(i = 0; i < count; i++)
            if(isValidChar(typed[i]) && nameIndex < (int) (sizeof(name) / sizeof(wchar_t)) - 1)
                name[nameIndex++] = typed[i];

        name[nameIndex] = 0;
    }
}
