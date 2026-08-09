#include "camera.h"

#define focalLength 0.05                /**< gyújtótáv [m] */
#define sensorSize 0.07                 /**< szenzor hosszabb oldalának a fele [m] */
#define renderObjectsPerCube 10         /**< max( (projectile + player + enemy) / m^3), erről később bővebben. */

vector3D cameraPosition;                /**< A kamera térbeli helyvektora. */
float magnification, columns, rows;    /**< Ennyi látszik a pályából [m]. */
bool watchBounds = false;              /**< Feldarabolós rajzolás van-e? */
float leftBound, rightBound, upperBound, lowerBound;    /**< Darabolás határai. */
int leftMostBound, rightMostBound, topBound, bottomBound;
renderObject*** renderMap = NULL;       /**< Darabolás mátrixa. */


///A feldarabolós rajzoláshoz szükséges mátrixot szabadítja fel.
void freeRenderMap() {
    if(renderMap != NULL) {
        free(renderMap[0][0]);
        free(renderMap[0]);
        free(renderMap);
    }
}

///A feldarabolós rajzoláshoz szükséges mátrixot mallocolja.
static void initRenderMap() {
    renderMap = (renderObject***) malloc(sizeof(renderObject**));
    renderMap[0] = (renderObject**) malloc(sizeof(renderObject*));
    renderMap[0][0] = (renderObject*) malloc(sizeof(renderObject));
}

/** \brief Kiszámolja a kamera poziciója és a képernyő mérete alapján, hogy mennyi látszik a pályából.
 *
 *  Elsősorban ezeket a változókat változtatja:
 *  columns, rows = képernyőn megjelenő oszlopok és sorok száma,
 *  magnification = megmondja egy méter a kamera "szenzorán" hány pixelnek felel meg (jó soknak)
 *
 *  Hasonló háromszögek alapján kiszámolja mennyi látszik a pályából:
 *       sensor size
 *         |
 *         V
 *       ----
 *       \  |
 *        \ | <- focalLength
 *         \|
 *          |
 *          |\
 *          | \
 *     Z -> |  \
 *          |   \
 *          |    \
 *    -------------
 *       columns
 *
 *  Ezek alapján reallocolja a darabolós mátrixot.
 *
 */
static void calculateRowsAndColumns() {
    magnification = (float) screen -> w / 2 / sensorSize;
    columns = 2 * cameraPosition.z * sensorSize / focalLength;
    //hogy a négyzet négyzet is maradjon
    rows = (float)screen -> h / screen -> w * columns;

    if(renderMap == NULL) initRenderMap();

    int xSize = (int)columns + 4;   //a nagyobb testek átlóghatnak a képernyő határán
    int ySize = (int)rows + 4;
    int iSize = renderObjectsPerCube;

    renderObject* innerTmp = (renderObject*) realloc(renderMap[0][0], xSize * ySize * iSize * sizeof(renderObject));
    renderObject** outerTmp = (renderObject**) realloc(renderMap[0], xSize * ySize * sizeof(renderObject*));
    renderMap = (renderObject***) realloc(renderMap, xSize * sizeof(renderObject**));

    renderMap[0] = outerTmp;

    int x;
    for(x = 1; x < xSize; x++)
        renderMap[x] = renderMap[0] + x * ySize;

    renderMap[0][0] = innerTmp;

    renderObject empty;
    empty.doesExist = false;

    int y, i = 0;
    int j;
    for(x = 0; x < xSize; x++)
        for(y = 0; y < ySize; y++) {
            renderMap[x][y] = renderMap[0][0] + i++ * iSize;
            for(j = 0; j < iSize; j++) renderMap[x][y][j] = empty;
        }
}

///A játékost követi a kamerával, nem hagyja, hogy a pálya alá lásson.
void followPlayerWithCamera() {
    renderObject player = getPlayerRenderObject();
    cameraPosition.x = (2 * player.position.x + player.size.x) / 2;
    cameraPosition.y = (player.position.y < rows / 2 ? rows / 2 : player.position.y) + 1;
}

///A menüben használt kamerát állítja be, visszaadja a columns-t és rows-t is.
void menuCamera(int* getColumns, int* getRows) {
    cameraPosition.z = cameraDeltaZDistanceStart;
    calculateRowsAndColumns();
    *getColumns = columns;
    *getRows = rows;
    cameraPosition.x = columns / 2;
    cameraPosition.y = rows / 2;
}

///Visszaadja, hogy hány oszlop látszik a pályából.
int getColumns() {
    return columns;
}

/** \brief A kamera dollyzását végzi, a kamerát mozgatja
 * A dolly állását staticban tárolja, ezért új dolly eseteén először egy restart = true-val kell meghívni,
 * \param ms eltelt idő
 * \param kell-e újrakezdés?
 * \return elérte-e a végállást?
 * Minden frameban újraszámolja a rows&coulmns-t,
 */
bool cameraDollyZ(int deltaTime, bool restart) {
    static int timePassed;
    if(restart) timePassed = 0;

    bool done = true;

    timePassed += deltaTime;

    float t = (float)(cameraDollyTime - timePassed) / cameraDollyTime;
    float cube = t*t*t;

    float currentZ = cameraDeltaZDistanceEnd + (cameraDeltaZDistanceStart - cameraDeltaZDistanceEnd) * cube;

    if(currentZ <= cameraDeltaZDistanceEnd) {
        currentZ = cameraDeltaZDistanceEnd;
        done = false;
    }

    cameraPosition.z = currentZ;
    calculateRowsAndColumns();
    return done;
}

/** \brief Térbeli pontból síkbelit képez.
 * A 3D-s pont koordinátái és a kamera alapján kiszámolja, hogy a kamera 2D-s szenzorán hol jelenik meg a pont.
 * Geometriailag leírva, a két ponton (point, cameraPosition) átmenő egyenes egyenletének, és a 'szenzor' vízszintesének metszéspontja
 * a keresett pontunk. A lent és a linken is látható egyenletet először a tér x-z, majd az y-z vetületére alkalmazzuk,
 * és megkapjuk a térbeli pontunk megfelelő síkbeli leképezést.
 * Ez interaktívan ábrázolva így néz ki: https://www.desmos.com/calculator/8qjvkptjmn
 * \param Térvektor típusú pont
 * \return Síkvektor típusú pont
 * A visszaadott pontnak int koordinátái vannak, hiszen valamelyik pixel koordinátáit jelöli.
 * Ez a függvény igazából egy camera obscura modellje.
 */
intVector2D get2DPoint(vector3D point) {
    //Ha watchBounds = true, csak egy megadott kockán belül adja vissza az értéket (rajzol), ez a feldarabolás miatt fontos
    if(watchBounds) {
        putInsideIntervalF(leftBound, &point.x, rightBound);
        putInsideIntervalF(lowerBound, &point.y, upperBound);
    }

    //Ezt a két kifejezést újra felhasználjuk
    float zDist = point.z - cameraPosition.z;
    float zDistFromHole = zDist - focalLength;
    float dx = (cameraPosition.x * zDistFromHole + focalLength * point.x) / zDist - cameraPosition.x;
    float dy = (cameraPosition.y * zDistFromHole + focalLength * point.y) / zDist - cameraPosition.y;

    //Megfelelően tükrözzük a kapot pontot, és a méterben kapott értéket átváltjuk pixelekre
    intVector2D resultPoint = {
        (int)(screen -> w / 2 - magnification * dx),
        (int)(screen -> h / 2 + magnification * dy)
    };

    return resultPoint;
}

/** \brief Lerajzol egy négyszöget
 * A bemenetként kapott 4 pont alkotta négyszöget rajzolja ki.
 * \param síkvektor a csúcshoz
 * \param síkvektor b csúcshoz
 * \param síkvektor c csúcshoz
 * \param síkvektor d csúcshoz
 * \param szín négyszög színe
 * Ezt egyszer kitöltéssel csinálja, egyszer pedig anti-aliasing poligonnal,
 * hogy a rajz széle ne legyen recegős.
 */
static void drawTetragon(intVector2D a, intVector2D b, intVector2D c, intVector2D d, color color) {
    Sint16 xCoords[4] = {(Sint16)a.x, (Sint16)b.x, (Sint16)c.x, (Sint16)d.x};
    Sint16 yCoords[4] = {(Sint16)a.y, (Sint16)b.y, (Sint16)c.y, (Sint16)d.y};

    filledPolygonRGBA(screen, xCoords, yCoords, 4, color.r, color.g, color.b, color.a);
        aapolygonRGBA(screen, xCoords, yCoords, 4, color.r, color.g, color.b, color.a);
}

/** \brief Lerajzol egy téglatestet
 * A kamera modul lényege, a rajzolást végzi el az eddigi függvények segítségével.
 * \param test téglatest adatai
 * \param bool melyik oldalakat kell rajzolni?
 * Végez egy plusz ellenőrést is, nem csak a paraméterben megadott tömb alapján nem rajzol oldalakat,
 * azokat se rajzolja le, amiket a helyzetük miatt biztosan nem láthat a kamera.
 */
void drawCuboid(renderObject object, bool sides[]) {
    intVector2D vertexes[8];
    //A téglatestet bejárva megkeressük a csúcsait
    int i;
    for(i = 0; i < 8; i++) {
        vertexes[i] = get2DPoint(object.position);
        switch(i % 4) {
        case 0:
            object.position.x += object.size.x;
            break;
        case 1:
            object.position.y += object.size.y;
            break;
        case 2:
            object.position.x -= object.size.x;
            break;
        case 3:
            object.position.y -= object.size.y;
        }
        if(i == 3) object.position.z += object.size.z;
    }

    //alja
    if(sides[0] && object.position.y > cameraPosition.y) drawTetragon(vertexes[0], vertexes[4], vertexes[5], vertexes[1], object.colors.dark);
    //bal oldali lap
    if(sides[1] && object.position.x > cameraPosition.x) drawTetragon(vertexes[0], vertexes[3], vertexes[7], vertexes[4], object.colors.dark);
    //jobb oldali lap
    if(sides[2] && object.position.x + object.size.x < cameraPosition.x) drawTetragon(vertexes[1], vertexes[2], vertexes[6], vertexes[5], object.colors.dark);
    //teteje
    if(sides[3] && object.position.y + object.size.y < cameraPosition.y) drawTetragon(vertexes[3], vertexes[7], vertexes[6], vertexes[2], object.colors.light);
    //eleje
    if(sides[4]) drawTetragon(vertexes[4], vertexes[5], vertexes[6], vertexes[7], object.colors.normal);
}

/// Az egy m^3-ben lévő testeket rajzolja le
static void drawPlace(int x, int* y, int direction, int bound) {
    bool sides[5] = {true, true, true, true, true};

    renderObject toDraw = getBlockRenderObject(x, y, direction, bound, sides);
    if(toDraw.doesExist) {
        watchBounds = false;
        drawCuboid(toDraw, sides);
        watchBounds = true;
    }
    else {
        int i = 0;
        toDraw = renderMap[x - leftMostBound][*y - bottomBound][i];
        while(toDraw.doesExist && i < renderObjectsPerCube) {
            drawCuboid(toDraw, sides);
            i++;
            toDraw = renderMap[x - leftMostBound][*y - bottomBound][i];
        }

        renderObject player = getPlayerRenderObject();

        if(
            (x == (int)floor(player.position.x) || x == (int)floor(player.position.x + player.size.x)) &&
            (*y == (int)floor(player.position.y) || *y == (int)floor(player.position.y + player.size.y))
        )
            drawCuboid(player, sides);
    }
}

///Az egy oszlopban lévő testeket rajzolja le, a részecskéket mozgatja közvetlen
static void drawY(int deltaTime, int x) {
    int y;

    for(y = topBound; y > cameraPosition.y; y--) {
        upperBound = y + 1;
        lowerBound = y;
        drawPlace(x, &y, -1, (int)cameraPosition.y);
    }
    for(y = bottomBound; y <= cameraPosition.y; y++) {
        upperBound = y + 1;
        lowerBound = y;
        drawPlace(x, &y, 1, (int)cameraPosition.y + 1);
    }
    watchBounds = false;
    drawParticles(x, bottomBound, topBound, deltaTime);
    watchBounds = true;
}

/** \brief Lerajzolja a teljes látható játékteret.
 * A megfelelő rajzoláshoz figyelni kell a testek takarására, nem mindegy ezért, hogy milyen sorrendben rajzoljuk őket.
 * A részecskéknél ezzel nincs probléma, azokat bármilyen sorrendben rajzolhatjuk. Ők a játékteret kitakarják, egymásnál viszont nem látszik,
 * hogy melyik takrja melyiket, ezért azokat elég a játéktér után bármilyen sorrendben lerajzolni.
 * A térképet alkotó kockáknál már nem ilyen egyszeű a helyzet. Azokat 4 irányból kell rajzolni, bal lentről, jobb lentről, bal fentről, illetve jobb fentről középre.
 * Ha ebben a négy lépésben rajzoljuk őket (ezek között a sorrend nem számít) akkor mindig a legfesőket rajzoljuk utoljára, és így valósnak néz ki az eredmény.
 * A játékos, a lövedékek, illetve az ellenségek viszont sajnos nincsenek egy négyzethálóhoz kötve. Így gyakran megesik, hogy az egyik felüket a pálya
 * egyik kockája előtt, a másikat pedig az után, de még a kövektező előtt kéne rajzolni. Ezt nem tudjuk megcsinálni, hatalmas erőforrásba kerülne ez a majdnem pixelről
 * pixelre történő szkennelés. A pályát továbbra is m^3-ént szeretnénk lerajzolni. Hogy ez megfelelően nézzen ki, minden köbméterben csak az abban a kockában lévő dolgokat
 * szeretnénk megrajzolni. Ehhez fel kell "darabolnunk" a játékost, a lövedékeket, illetve az ellenségeket is. A darabolás azt jelenti, hogy egy mátrix minden cellájába
 * beírjuk, hogy az adott kockában melyik, milyen tulajdonsággal rendelkező testek lógnak be. Rajzolásnál pedig a testeknek mindig csak az adott kockabeli részét rajzoljuk le.
 * Mivel a kockákat jó sorrendben rajzoljuk, így a bennük levő "testekrészeket" is jó sorrendbe fogjuk rajzolni. A problémát megoldottuk.
 * \param ms eltelt idő
 * Ez a függvény elvégzi a részecskék, a pálya, illetve minden más test rajzolását is. A részecskék mozgatása innen van meghívva.
 */
void drawMap(int deltaTime) {
    leftMostBound = (int)(cameraPosition.x - (float)columns / 2) - 1;
    rightMostBound = (int)(cameraPosition.x + (float)columns / 2) + 1;
    topBound = (int)(cameraPosition.y + (float)rows / 2 + 1);
    bottomBound = (int)(cameraPosition.y - (float)rows / 2 - 1);

    int y, x;

    for(x = leftMostBound; x < rightMostBound; x++)
        for(y = bottomBound; y < topBound; y++)
            getFloatersRenderObject(x, y, renderMap[x - leftMostBound][y - bottomBound], renderObjectsPerCube);

    float slicer = cameraPosition.x;
    watchBounds = true;
    for(x = leftMostBound; x <= slicer; x++) {
        leftBound = x - 0.005;
        rightBound = x + 1 + 0.005;
        drawY(deltaTime, x);
    }
    for(x = rightMostBound; x > slicer - 1; x--) {
        leftBound = x - 0.005;
        rightBound = x + 1 + 0.005;
        drawY(deltaTime, x);
    }

    watchBounds = false;
}
