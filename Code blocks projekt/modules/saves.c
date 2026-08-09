#include "saves.h"


///Megnézi, melyik save slotok foglaltak, ha nem NULL pointert kap, megnézni melyik mentés milyen régi, és hány pontja van benne a játékosnak.
void getExistingSaveFiles(time_t ages[], int points[]) {
    int saveID = 0;
    char save[50];
    FILE* saveFile;
    while(saveID < maxNOSaves) {
        sprintf(save, "saves\\game%d.save", saveID);
        saveFile = fopen(save, "r");
        if(saveFile != NULL) {
            existingSaves[saveID] = true;
            if(saveFile != NULL) {
                if(ages != NULL) {
                    fscanf(saveFile, "%ld,", &(ages[saveID]));
                    int i;
                    for(i = 0; i < 8; i++)
                        fscanf(saveFile, "%d,", &(points[saveID]));
                }
                fclose(saveFile);
            }
        }
        else existingSaves[saveID] = false;
        saveID++;
    };
}

///Mentés törlése (fizikálisan).
void deleteSave(int i) {
    char save[50];
    sprintf(save, "saves\\game%d.save", i);
    remove(save);
}

/** \brief Elmenti a jelenlegi játékállást (kivéve a floatereket, de ez nem hiányosság, hanem a játék filozófiájának része).
 * A procedúra gyorsítása végett buffert és tömörítést használ. A mentés fájl első pár eleme a beállításokat, és a játékos adatait tartalmazza.
 * A mentés időpontja is a fájlban tárolódik. A tömörítés ezután következik: A számok (amik az block enumból jönnek) előtt zárójelben ott van, hogy
 * az vízszintesen egymásután hányszor fordul elő. Tehát a (102)1, azt jelenti, hogy egymás után 102 db egyes van. Ez igen jelentősen csökkenti a
 * mentések méretét. Gyorsabb lenne, ha nem karakterek formájában menne a fájlba írás, de így is elég gyors, szóval egyelőre jó lesz.
 * \param beállítások
 * A mentés fájl tartalmazza a pálya dimenzióit, "sortörések" nincsenek a fájlban, ha nem megfelelő formátumú fájlt próbálunk betölteni jó esetben
 * szétcsúszik a pálya, roszabban pedig túlírunk a memóriában. Szóval a felhasználó ne nagyon szerkesztgesse a mentéseket.
 */
void saveGame(int currentOptions[]) {
    char saveName[50];
    getExistingSaveFiles(NULL, NULL);
    int i;
    for(i = 0; i < maxNOSaves - 1 && existingSaves[i]; i++)
        ;
    sprintf(saveName, "saves\\game%d.save", i);
    FILE* save = fopen(saveName, "w");

    block** map = getMap();

    fprintf(save, "%ld,", time(NULL));
    for(i = 0; i < 5; i++)
        fprintf(save, "%d,", currentOptions[i]);

    renderObject player = getPlayerRenderObject();
    fprintf(save, "%d,%d,%d,", (int)(player.position.x * 10), (int)(player.position.y * 10), getPoints());

    char* buffer = (char*) malloc(bufferSize);
    int bufferIndex = 0;
    char temp[20];
    int x, y;
    int repCount;
    for(y = currentOptions[1] - 1; y >= 0; y--) {
        for(x = 0; x < currentOptions[0]; x++) {
            for(repCount = 1; x + repCount < currentOptions[0] && map[x][y] == map[x + repCount][y]; repCount++)
                ;
            if(repCount > 4) {
                x += repCount - 1;

                sprintf(temp, "(%d)%d,", repCount, map[x][y]);
                strcpy(buffer + bufferIndex, temp);
                bufferIndex += strlen(temp);
            }
            else {
                sprintf(temp, "%d,", map[x][y]);
                strcpy(buffer + bufferIndex, temp);
                bufferIndex += strlen(temp);
            }
            if(bufferIndex > bufferSize - 20) {
                fprintf(save, buffer);
                bufferIndex = 0;
            }
        }
    }
    if(bufferIndex != 0)
        fprintf(save, buffer);
    free(buffer);

    fclose(save);
}

///Fájlból betölti a játékot, a világot is előkészíti. A mentésnél használt tömörítést dekódolja. A betöltött játék beállítások felülírják az eddigieket.
void loadGame(int currentOptions[], int saveID) {
    char saveName[50];
    sprintf(saveName, "saves\\game%d.save", saveID);

    FILE* save = fopen(saveName, "r");

    time_t tmpT;
    fscanf(save, "%ld,", &tmpT);

    int i;
    for(i = 0; i < 5; i++)
        fscanf(save, "%d,", &(currentOptions[i]));

    int playerX, playerY;
    int points;

    fscanf(save, "%d,%d,%d,", &playerX, &playerY, &points);

    setPlayerStartingPosition(playerX / 10.0, playerY / 10.0);
    setPoints(points);

    setMapSize(currentOptions[0], currentOptions[1]);
    generateColorMap();

    g = currentOptions[3] / 10e6;

    vector2D maxVelocity;
    getPlayerStats(&maxVelocity);

    createMap();

    block** map = getMap();

    int x, y, repCount;
    int blockToWrite;
    for(y = currentOptions[1] - 1; y >= 0; y--) {
        for(x = 0; x < currentOptions[0]; x++) {
            if(fscanf(save, "(%d)%d,", &repCount, &blockToWrite) > 0) {
                for(i = 0; i < repCount; i++)
                    map[x + i][y] = blockToWrite;
                x += repCount - 1;
            }
            else {
                fscanf(save, "%d,", &blockToWrite);
                map[x][y] = blockToWrite;
            }
        }
    }

    setParticleMultiplier(currentOptions[2], false);
    createParticleList();
    freeFloaterList();

    fclose(save);
}
