#include "menuDraw.h"


///A menüben lévő havazást kezeli, sokkal olcsóbb, mintha egy mapDrawt hívnánk.
static void drawMenuParticles(int deltaTime) {
    int columns = getColumns();
    int i;
    for(i = 0; i < columns; i++)
        drawParticles(i, 0, columns, deltaTime);
}

///A főmenüt rajzolja.
void drawMainMenu(int deltaTime) {
    wchar_t menuOptions[][50] = {
        L"Indítás",
        L"Betöltés",
        L"Beállítások",
        L"Ranglista",
        L"Leírás",
        L""
    };

    unicodePrintCentered(L"Platform játék", 50, 20, large);

    int i;
    for(i = 0; menuOptions[i][0] != '\0'; i++)
        unicodePrintCentered(menuOptions[i], 50, menuStart + i * menuTav, isMouseOverRect(50, menuStart + menuTav*i, 15, menuTav / 2) ? smallBold : small);

    drawMenuParticles(deltaTime);
}

///A "vissza" gombot rajzolja
static inline void printBack() {
    unicodePrintCentered(L"Visszalépés", 50, menuStart + 5 * menuTav, isMouseOverRect(50, menuStart + menuTav*5, 15, menuTav / 2) ? smallBold : small);
}

///A beállítások menüpont rajzolása, a beállításokat a betöltött mentések is befolyásolják. Figyelni kell, hogy a belépés kattintását ne érzékelje csúszka kattintásnak.
void drawOptionsMenu(int deltaTime, int currentOptions[], bool click) {
    wchar_t optionsMenuOptions[][50] = {
        L"Világ szélessége",
        L"Világ magassága",
        L"Részecskék száma",
        L"Gravitáció",
        L"Ellenségek közti idő",
        L""
    };

    int minOptions[] = {  10,   20,  0,   150,  2000};
    int maxOptions[] = {3000, 3000, 50,   800, 20000};

    int i = 0;
    while(optionsMenuOptions[i][0] != '\0') {

        unicodePrintCentered(optionsMenuOptions[i], 50, optionsStart + (i - 0.3) * optionsTav, tiny);
        lineColor(
            screen,
            0.3 * screen -> w, (float) screen -> h / 100 * (optionsStart + i * optionsTav),
            0.7 * screen -> w, (float) screen -> h / 100 * (optionsStart + i * optionsTav), 0xFFFFFFFF
        );

        filledCircleColor(screen, (float) screen -> w / 100 * (30 + 0.4 * 100 * (float)(currentOptions[i] - minOptions[i]) / (maxOptions[i] - minOptions[i])),
            (float) screen -> h / 100 * (optionsStart + i * optionsTav), larger(4, (int)(20 * uiScale())), 0xFFFFFFFF
        );
        aacircleColor(screen, (float) screen -> w / 100 * (30 + 0.4 * 100 * (float)(currentOptions[i] - minOptions[i]) / (maxOptions[i] - minOptions[i])),
           (float) screen -> h / 100 * (optionsStart + i * optionsTav), larger(4, (int)(20 * uiScale())), 0xFFFFFFFF
        );

        i++;
    }

    if(click) {
        for(i = 0; i < 5; i++) {
            if(isMouseOverRect(50, optionsStart + optionsTav * i, 50, optionsTav / 2)) {
                //Ezek nem magic számok, csak egy kicsit.
                currentOptions[i] = minOptions[i] + (int)((maxOptions[i] - minOptions[i]) * (getMousePosition().x - screen -> w * 0.3) / (screen -> w * 0.4) );
                putInsideInterval(minOptions[i], &currentOptions[i], maxOptions[i]);
            }
        }
    }

    printBack();
    drawMenuParticles(deltaTime);
}

///A játék leírását rajzolja meg.
void drawInfoMenu(int deltaTime) {

    wchar_t info[][150] = {
        L"Ebben a játékban pontokat szerezni és költeni",
        L"is tudsz. Minden megtalált kék kockáért jár egy pont.",
        L"Mozogni a WAD billentyűk, szóköz, vagy a nyilak",
        L"segítségével tudsz. A bal egérgombbal egy pontért",
        L"cserébe lőhetsz. A jobb egérgombbal lelassíthatod",
        L"az időt, természetesen ez sincs ingyen. A repülő piros",
        L"kockák az ellenségek, ha hozzád érnek a játéknak vége.",
        L"A játékot az ESC megnyomásával szüneteltetheted.",
        L""
    };

    int i = 0;
    while(info[i][0] != '\0') {
        unicodePrintCentered(info[i], 50, infoStart + i * infoTav, tiny);
        i++;
    }

    printBack();
    drawMenuParticles(deltaTime);
}

static wchar_t names[topListItemsMax][100];
static int points[topListItemsMax];
static int namesIndex;

///Ha megtelt a toplista, rendezi, és az utolsó elemét lecseréli. A ranglista fájlt se kéne nagyon piszkálni a felhasználónak.
static void sortTopList() {

    int i, j;
    int tmpInt;
    wchar_t tmpStr[100];

    for(i = 0; i < namesIndex; i++) {
        int maxIndex = i;
        for(j = i + 1; j <= namesIndex; j++)
            if(points[maxIndex] < points[j])
                maxIndex = j;

        tmpInt = points[i];
        points[i] = points[maxIndex];
        points[maxIndex] = tmpInt;

        wcscpy(tmpStr, names[i]);
        wcscpy(names[i], names[maxIndex]);
        wcscpy(names[maxIndex], tmpStr);
    }
}

///A toplista menü előtt meg kell hívni, hogy rendezett létező toplistát lássunk.
void loadTopList() {
    FILE* topList = fopen("rang.lst", "r");

    int i;

    if(topList != NULL) {
        char lineName[400];
        namesIndex = 0;
        while(fscanf(topList, " %399[^,\n],%d", lineName, &points[namesIndex]) == 2) {
            i = strlen(lineName);
            lineName[i] = ',';
            lineName[i + 1] = 0;
            utf8ToWide(lineName, names[namesIndex], 100);

            if(namesIndex < topListItemsMax - 1) namesIndex++;
            else sortTopList();
        }
        sortTopList();
        fclose(topList);
    }
}

///Maga a toplista razoló függvény, az első pár helyet nagyobb betűvel rajzolja.
void drawTopList(int deltaTime) {
    int i;
    for(i = 0; i < namesIndex; i++) {
        char storedName[400], nameWScore[600];
        wideToUtf8(names[i], storedName, sizeof(storedName));
        snprintf(nameWScore, sizeof(nameWScore), "%s %d", storedName, points[i]);
        utf8PrintCentered(nameWScore, 50, 20 + ((i == 0) ?  0 : (i + 1)*infoTav), (i == 0) ? large : (i == 1) ? small : tiny);
    }

    printBack();
    drawMenuParticles(deltaTime);
}

///Ha a játékos szüneteltei a játékot(ESC) ezt fogja látni. Elég drága az átlátszó box, szóval csak óvatosan a méretével.
void drawPauseMenu(bool saved) {
    wchar_t pauseOptions[][100] = {
        L"Játék folytatása",
        L"Pálya elmentése",
        L"Mentés betöltése",
        L"Kilépés a főmenübe",
        L"",
        L"Elmentve"
    };

    boxRGBA(screen, screen -> w * 0.3, screen -> h * 0.3, screen -> w * 0.7, screen -> h * 0.7, 0, 0, 0, 200);

    int i;
    for(i = 0; pauseOptions[i][0] != '\0'; i++) {
        if(i == 1 && saved) unicodePrintCentered(pauseOptions[5], 50, menuStart - 5 + i * menuTav, isMouseOverRect(50, (menuStart - 5) + menuTav*i, 15, menuTav/2) ? smallBold : small);
        else unicodePrintCentered(pauseOptions[i], 50, menuStart - 5 + i * menuTav, isMouseOverRect(50, (menuStart - 5) + menuTav*i, 15, menuTav/2) ? smallBold : small);
    }
}

///Itt lehet mentett játékot betölteni, ha belépünk ebbe a menübe, a jelenlegi játékmenetünk elveszik.
void drawLoadMenu(int deltaTime) {
    time_t ages[maxNOSaves];
    int points[maxNOSaves];
    getExistingSaveFiles(ages, points);


    int i;
    const char* timeStrings[] = {
        "másodperccel",
        "perccel",
        "órával",
        "nappal",
        "héttel",
        "hónappal",
        "évvel"
    };
    int timeConversion[] = {
        1, 60, 60*60, 60*60*24, 60*60*24*7, 60*60*24*30, 60*60*24*365
    };
    int timeStringsIndex;
    int k =0;
    for(i = 0; i < maxNOSaves; i++) {
        if(existingSaves[i]) {
            int deltaRealTime = time(NULL) - ages[i];

            for(timeStringsIndex = 0; timeStringsIndex < 6; timeStringsIndex++)
                if(deltaRealTime < timeConversion[timeStringsIndex+1])
                    break;

            int elapsedSaveTime = deltaRealTime / timeConversion[timeStringsIndex];
            char save[300];
            snprintf(save, sizeof(save), "%d %s ezelőtti mentés, %d ponttal", elapsedSaveTime, timeStrings[timeStringsIndex], points[i]);
            utf8PrintCentered(save, 50, loadStart + k * loadTav, isMouseOverRect(50, loadStart + loadTav*k, 25, 3) ? smallBold : small);
            unicodePrintCentered(L"Törlés", 50, loadStart + k * loadTav + 5, isMouseOverRect(50, loadStart + loadTav*k + 5, 25, 2) ? smallBold : small);
        k++;
        }
    }
    printBack();
    drawMenuParticles(deltaTime);
}
