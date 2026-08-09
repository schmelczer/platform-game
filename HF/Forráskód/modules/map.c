#include "map.h"

#define maxPlatformLength 5     /**< A legszélesebb platformok szélessége [m]. */
#define minPlatformLength 2     /**< A legkeskenyebb platformok szélessége [m], ez nem teljesen igaz, mindig lesznek ksiebbek is, csak nem olyan sok. */
#define baseHeightPercent 0.3   /**< A pálya hányad része legyen tömör "föld". */
#define fuzziness  2            /**< Mennyire legyen recegős a pálya? (legnagyobb magasság különbségek a földszinten)*/
#define treasureColor (colors){ (color){69, 82, 160, 255}, (color){58, 69, 135, 255}, (color){44, 53, 102, 255} } /**< A jutalmak színe. */
#define treasureSize (vector3D){0.5, 0.5, 0.25} /**< A jutalom mérete. */

colors* colorMap = NULL;        /**< A színeket tartalmazó 1 dimenziós tömb. (minden oszlop egyszínű) */
block** map = NULL;             /**< A pályát tartalmazó 2 dimenziós tömb. */
intVector2D mapSize;            /**< A pálya mérete [m]. */


///A pálya méretét lehet vele beállítani.
void setMapSize(int x, int y) {
    mapSize.x = x;
    mapSize.y = y;
}

///A pálya méretét adja vissza.
intVector2D getMapSize() {
    return mapSize;
}

///A pályát adja vissza.
block** getMap() {
    return map;
}

///Megmondja, hogy egy pont benne van-e egy négyzetben.
bool isPointWithinBox(vector2D point, vector2D boxPosition, vector2D boxSize) {
    return
        boxPosition.x < point.x && point.x < boxPosition.x + boxSize.x &&
        boxPosition.y < point.y && point.y < boxPosition.y + boxSize.y;
}

///Megmondja, hogy két négyzet fedésben van-e.
bool isBoxOverlappingBox(vector2D box1Position, vector2D box1Size, vector2D box2Position, vector2D box2Size) {
    int i;
    for(i = 0; i < 4; i++) {
        switch(i) {
            case 1:
                box1Position.x += box1Size.x;
                break;
            case 2:
                box1Position.y += box1Size.y;
                break;
            case 3:
                box1Position.x -= box1Size.x;
        }
        if(isPointWithinBox(box1Position, box2Position, box2Size)) return true;
    }
    return false;
}

///Megmondja, hogy egy pont a pályán belül van-e.
bool isWithinTheMap(vector2D point, vector2D size) {
    return
        0 <= point.x && point.x + size.x < mapSize.x &&
        0 <= point.y && point.y + size.y < mapSize.y;
}

///Megmondja, hogy történt-e ütközés egy négyzet és a pálya között. Visszaadja a szóban forgó kocka pointerét is.
bool detectCollision(vector2D position, vector2D size, block collidableBlock, block** collidedWith) {
    int x, y;
    if (isWithinTheMap(position, size)) {
        for(x = (int)position.x; x < position.x + size.x; x++) {
            for(y = (int)position.y; y < position.y + size.y; y++) {
                *collidedWith = &map[x][y];
                if(**collidedWith == collidableBlock) return true;
            }
        }
        return false;
    }
    else {
        collidedWith = NULL;
        return true;
    }
}

///Felszabadítja a színek tömbjét.
void freeColorMap() {
    free(colorMap);
}

///Véletlenszerűen létrehozza a kockák palettáját, egy oszlopon belül minden kocka egyszínű.
void generateColorMap() {
    freeColorMap();

    colorMap = (colors*) malloc(mapSize.x * sizeof(colors));
    int x;
    for(x = 0; x < mapSize.x; x++)
        colorMap[x] = createBlockColor();
}

///Felszabadítja a térkép tömbjét.
void freeMap() {
    if(map != NULL) {
        free(map[0]);
        free(map);
    }
}

///Létrehoz egy tömböt a térképnek.
void createMap() {
    freeMap();

    int x;
    map = (block**) malloc(mapSize.x * sizeof(block*));
    map[0] = (block*) malloc(mapSize.x * mapSize.y * sizeof(block));
    for(x = 1; x < mapSize.x; x++)
        map[x] = map[0] + x * mapSize.y;
}

///Kiszámolja, hogy ellehet-e érni egy adott kockát a játékosnak.
static bool canBeReached(int deltaX, int deltaY, vector2D maxVelocity, float hMax) {
    float t1 = maxVelocity.y / g;
    float t2 = sqrt(2 * (hMax - (float)deltaY) / g);
    float possibleDeltaX = maxVelocity.x * (t1 + t2);

    return possibleDeltaX > deltaX;
}

///Egy kiszámolt mátrix alapján megmondja, hogy a játékos elérhet-e egy adott kockát.
static bool canJumpThere(intVector2D from, intVector2D to, int width, int height, bool possiblePlacesToJumpTo[][height]) {
    int xIndex;
    int yIndex = to.y - from.y - 1;
    if(yIndex >= height || yIndex < 0) return false;

    xIndex = to.x > from.x ? to.x - from.x - 1 : from.x - to.x - 1;
    if(xIndex >= width || xIndex < 0) return false;

    return possiblePlacesToJumpTo[xIndex][yIndex];
}

///A pálya földszintjét hozza létre.
static void createMapBase() {
    int x, y, currentHeight;

    int baseHeight = (int) (mapSize.y * baseHeightPercent);

    for(x = 0; x < mapSize.x; x++)
        for(y = 0; y < mapSize.y; y++)
            map[x][y] = air;

    currentHeight = baseHeight / 2;
    for(x = 0; x < mapSize.x; x++) {

        if(fuzziness > 0) {
            currentHeight = randomIntWithinBorders(currentHeight, fuzziness);
            putInsideInterval(1, &currentHeight, baseHeight);
        }

        for(y = 0; y <= baseHeight + currentHeight; y++)
            map[x][y] = solid;
    }
}

///Létrehoz egy platformot, ami nem lóg túl a pályarészek határán, ezekről később bővebben.
static void buildPlatform(int direction, intVector2D* previous, int x, int xMaxBound) {
    int platformLength = rand() % (maxPlatformLength - minPlatformLength) + minPlatformLength + 1;

    if(direction == 1 && previous -> x + platformLength - 1 >= xMaxBound)
        platformLength = xMaxBound - previous -> x;

    if(direction == -1 && previous -> x - platformLength < x)
        platformLength = previous -> x - x + 1;

    int m;
    for(m = 0; m < platformLength; m++) {
        map[previous -> x + direction * m][previous -> y] = solid;
        if(rand() % 3 == 0) map[previous -> x][previous -> y + 1] = treasure;
    }

    previous -> x += direction * rand() % platformLength;
}

///Primitív megoldás, de lthatatlan falakat csinál, amik megakadályozzák, hogy olyan helyre menjen a játékos, ahova nem kéne.
static void createInvisibleBounds() {
    int x, y;
    for(x = 0; x < mapSize.x; x++) {
        map[x][0]= invisible;
        map[x][mapSize.y - 1]= invisible;
    }

    for(y = 0; y < mapSize.y; y++) {
        map[0][y]= invisible;
        map[mapSize.x - 1][y]= invisible;
    }
}

/** \brief Térkép platformokkal betöltése, úgy hogy játszható legyen
 * Kisebb egységekre, részekre bontja a pályát. Minden egységben egy darab platform oszlop van. A platformok magasság különbsége olyan,
 * hogy a jelenlegi fizikával a játékos ugrálva elérhesse őket. A pályagenerálás előtt ezért ki kell számolni, milyen messzire ugorhat.
 * \param sebességvektor player max
 * A platformokra random tesz jutalmakat (kék kockákat) is.
 * Nem túl hosszú a függvény, de nem is a legrövidebb, ennél több részre sajnos nem lehet szétszedni szépen.
 */
void generateMap(vector2D maxVelocity) {

    createMapBase();

    int x, y;

    //A játékos képességeinek maximuma
    float realHMax = (maxVelocity.y * maxVelocity.y / 2 / g);
    int lowerHMax = (int)realHMax;
    int upperHMax = lowerHMax + 1;
    int lowerXMax = (int)(2 * maxVelocity.x * maxVelocity.y / g);


    //Hova ugorhat a (-1, -1) -ről?
    int possibleWidth = lowerXMax;
    int possibleHeight = lowerHMax;
    bool possiblePlacesToJumpTo[possibleWidth][possibleHeight];


    for(y = 0; y < possibleHeight; y++)
        for(x = 0; x < possibleWidth; x++)
            possiblePlacesToJumpTo[x][y] = canBeReached(x + 1, y + 1, maxVelocity, realHMax);


    intVector2D min, max, previous, test;

    int previousPreviousY;
    bool isOkey;

    int xInterval = 10;
    int minHeightDiff = 2;
    int l, direction, optionsCount, xMaxBound;
    bool alreadyTried;

    intVector2D options[possibleWidth * possibleHeight];

    for(x = 0; x < mapSize.x; x += xInterval) {

        //Kezdőpont keresés
        previous.x = smaller(x + rand() % xInterval, mapSize.x - 1);
        xMaxBound = smaller(x + xInterval, mapSize.x);
        min.y = getHeightAtX(previous.x);

        previous.y = min.y;
        previousPreviousY = -100;
        max.y = min.y;

        //Addig csináljuk, amíg nem érünk a tetejére
        while(previous.y + 2 * upperHMax < mapSize.y) {

            //Határok meghatározása véletlenszerű irány alapján
            direction = randomPlusOrMinus();
            alreadyTried = false;

            newDirection:

            min.y = larger(previousPreviousY + upperHMax + 1, previous.y + minHeightDiff) + 1;
            max.y = previous.y + possibleHeight;
            //[határok)
            if(direction == 1) {
                //jobbra
                min.x = smaller(previous.x + 1, mapSize.x - 1);
                max.x = smaller(previous.x + possibleWidth + 1, xMaxBound);
            }
            else {
                //balra
                min.x = larger(larger(previous.x - possibleWidth, x), 0);
                max.x = larger(previous.x, 1);
            }

            //A kiválasztott határok közé lehet-e ugrani egyáltalán?
            isOkey = false;
            optionsCount = 0;
            for(test.x = min.x; test.x < max.x; test.x++)
                for(test.y = min.y; test.y < max.y; test.y++)
                    if(canJumpThere(previous, test, possibleWidth, possibleHeight, possiblePlacesToJumpTo)) {
                        isOkey = true;
                        options[optionsCount++] = test;
                    }
            //Ha igen, válasszunk ki egyet véletlenszerűen
            if(isOkey) {
                previousPreviousY = previous.y;
                l = rand() % optionsCount;
                previous = options[l];

                buildPlatform(direction, &previous, x,  xMaxBound);
            }
            else {
                if(alreadyTried) break;
                alreadyTried = true;
                direction *= -1;
                goto newDirection; // a goto nem a legjobb, de mindenképpen szebb egy for ciklusnál
            }
        }
    }
    createInvisibleBounds();
}

///Megmondja egy adott x-hez milyen magas földszint tartozik
int getHeightAtX(int x) {
    int y;
    for(y = 1; map[(int)x][y] == solid; y++)
        ;
    return y;
}

///Ez a player modul szempontjából lényeges, megmondja, hogy belelóg-e egy kockába
bool isInBlockSideways(int x, int lowestY, int uppermostY) {
    return map[x][lowestY]== solid || map[x][lowestY]== invisible ||
        map[x][uppermostY]== solid || map[x][uppermostY]== invisible;
}

///Ez a player modul szempontjából lényeges, megmondja, hogy belelóg-e egy kockába
bool isInBlockPerpendicular(int leftmostX, int rightmostX, int y) {
    return map[leftmostX][y]== solid || map[leftmostX][y]== invisible ||
        map[rightmostX][y]== solid || map[rightmostX][y ]== invisible;
}

///Melyik oldalai látszanak egy kockának? Azok biztos nem, amiket más kockák eltakarnak.
static char calculateSides(bool sides[], int x, int y) {
    sides[0] = map[x][y - 1] != solid;      //alja
    sides[1] = map[x - 1][y] != solid;   //bal oldal
    sides[2] = map[x + 1][y] != solid;  //jobb oldal
    sides[3] = map[x][y + 1] != solid;      //teteje
    sides[4] = true;    //eleje

    char sum = 0;
    int i;
    for(i = 0; i < 5; i++)
        if(sides[i]) sum += 1 << i;
    return sum;
}

/** \brief A térkép rajzolást teszi lehetővé.
 * A sebesség növelés végett az egymás alatt lévő kockákat (mivel egyszínűek) egyszerre is rajzolhatjuk.
 * Ilyenkor több dologra is kell figyelnünk. Először is, nem elég az azonos szín, arra is figyelni kell, hogy
 * azonos oldalaik is látszódjanak. Ha tud oszlopot rajzolni, akkor az y koordját módosítja hívójának.
 * \param koord x
 * \param koord* y
 * \return test oszlop
 * Pályakockákat és jutalmakat is rajzol.
 */
renderObject getBlockRenderObject(int x, int* y, int direction, int bound, bool sides[]) {
    renderObject blockRenderObject;
    if(isWithinTheMap((vector2D){x, *y}, (vector2D){1, 1}) && map[x][*y] == treasure) {
        calculateSides(sides, x, *y);
        sides[1] = true;
        sides[2] = true;
        sides[3] = true;

        //jutalmat rajzol
        blockRenderObject.position = (vector3D){x + 0.25, *y, 0.375};
        blockRenderObject.size = treasureSize;
        blockRenderObject.colors = treasureColor;
        blockRenderObject.doesExist = true;

    }
    else if(isWithinTheMap((vector2D){x, *y}, (vector2D){1, 1}) && map[x][*y] == solid) {
        char sidesSum = calculateSides(sides, x, *y);
        int i = 1;
        while(((direction == -1 && *y - i > bound) || (direction == 1 && *y + i < bound)) &&
              map[x][*y + i * direction] == solid &&
              sidesSum == calculateSides(sides, x, *y + i * direction))
            i++;

        sidesSum = calculateSides(sides, x, *y);

        blockRenderObject.size = (vector3D){1, i, 1};
        int newY = direction == 1 ? *y : *y - (i-1);
        blockRenderObject.position = (vector3D){x, newY, 0};
        blockRenderObject.colors = colorMap[x];
        blockRenderObject.doesExist = true;

        *y += direction * (i - 1);
    }
    else {
        blockRenderObject.doesExist = false;
    }
    return blockRenderObject;
}
