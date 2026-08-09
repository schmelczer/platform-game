#include "world.h"

///A menüben használt kamera értékeit állítja be, és csinál hóesést.
void createMenuWorld() {
    int columns, rows;
    menuCamera(&columns, &rows);
    setMapSize(columns, rows);
    setParticleMultiplier(2, true);
    createParticleList();
}

///Random generál világot, a mentés betöltésnél nem ezt használjuk.
void createWorld(int options[]) {
    setMapSize(options[0], options[1]);

    generateColorMap();
    setPoints(10);
    g = options[3] / 10e6;

    vector2D maxVelocity;
    getPlayerStats(&maxVelocity);

    createMap();
    generateMap(maxVelocity);

    setPlayerStartingPosition(getMapSize().x / 2, getHeightAtX(getMapSize().x / 2));

    setParticleMultiplier(options[2], false);

    createParticleList();

    freeFloaterList();
}
