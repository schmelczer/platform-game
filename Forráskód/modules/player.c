#include "player.h"

#define playerSize (vector3D) {0.4, 0.9, 0.25}  /**< A játékos méretét reprezentáló térvektor [m]. */
#define playerMaxVelocity (vector2D) {0.0045, 0.03}  /**< A max sebességet [m/ms] sehogy se tudja túllépni, a program nagyságrendekkel nagyobbakat nem is kezelne jól. */
#define playerColors (colors) { (color) {229, 22, 22, 255}, (color) {200, 20, 20, 255}, (color) {168, 16, 16, 255} } /**< Hasonló elven 3 féle árnyalat, mint a pályaelemeknél is. */
#define playerXAcceleration 0.00005 /**< Mintha lenne surlódás/légellenállás, sok értelme nincs, de kicsit lágyabb lesz tőle a mozgás. */

vector3D playerPosition;        /**< A játékos helyvektora, 3d-s, mert nem z = 0 pozban van [m]. */
vector2D playerVelocity;        /**< A játékos sebesség [m/ms], a delta time miatt szükség van rá. */
int directionOfMovement = 0;    /**< A mozgás iránya, lehet -1, 0, +1. */
bool isJumping = false, isFalling = true;
bool isDead = false;
static int points;              /**< Mennyi pontot gyűjtött eddig a játékos. Ez nőhet és gyökkenhet is, de mindig >= 0. */


void setDie(bool newValue) {
    isDead = newValue;
}

bool isAlive() {
    return !isDead;
}

void updateText(int newPoints) {
    char topRightCornerText[51];
    sprintf(topRightCornerText, "Pontszámod: %d", newPoints);
    SDL_FreeSurface(text);
    text = TTF_RenderUTF8_Blended(fontSmall, topRightCornerText, (SDL_Color) {255, 255, 255});
}

void drawPointsText() {
    if(!isRenderingEnabled()) return;
    SDL_BlitSurface(text, NULL, screen, &(SDL_Rect){50, 30, 0, 0});
}

void setPlayerStartingPosition(int x, int y) {
    playerPosition = (vector3D) {x, y, (1 - playerSize.z) / 2};
    updateText(points);
}

void setPoints(int newAmount) {
    points = newAmount;
    updateText(points);
}

int getPoints() {
    return points;
}

/** \brief Mozgatja a játékost a billentyű lenyomások alapján.
 * Az ütközés érzékelés elég érdekes, megnézni, hogyha megváltoztatná a játékos helyzetét, akkor illegális pozicióba lenne-e, ha igen akkor
 * pont annyira változtatja meg, hogy épp ne legyen illegális állapotban.
 * \param billentyűzet
 * \param ms eltelt idő
 */

void movePlayer(unsigned char* keys, int deltaTime) {

    //nem a legszebb, de a rövidségtől szép lesz.
    directionOfMovement = (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) - (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]);

    isJumping = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_SPACE];
    //<\input feldolgozása>
    //<sebesség kiszámítása>
    if(directionOfMovement != 0) {
        //billentyű lenyomás hatására elkezd gyorsulni, irányváltáskor pedig lefékez, majd megfordul
        playerVelocity.x +=  directionOfMovement * playerXAcceleration * deltaTime;
        //hogy ne lehesen gyorsabb a max sebességénél
        if(playerVelocity.x >  playerMaxVelocity.x)  playerVelocity.x =  playerMaxVelocity.x;
        else if(playerVelocity.x < -playerMaxVelocity.x)  playerVelocity.x = - playerMaxVelocity.x;
    }
    else {
    //lassulva megáll
        if(playerVelocity.x > 0) {
            playerVelocity.x -= playerXAcceleration * deltaTime;
            //a sebesség osszcilálását elkerülendő
            if(playerVelocity.x < 0)  playerVelocity.x = 0;
        }
        if(playerVelocity.x < 0) {
            playerVelocity.x += playerXAcceleration * deltaTime;
            if(playerVelocity.x > 0)  playerVelocity.x = 0;
        }
    }
    //</sebesség kiszámítása>
    //<új x hely kiszámítása>
    int lowestY = (int)floor(playerPosition.y);
    int uppermostY = (int)floor(playerPosition.y +  playerSize.y);

	if(playerVelocity.x != 0) {
	    float newX =  playerPosition.x +  playerVelocity.x * deltaTime;

	    int newRightmostX = (int)floor(newX +  playerSize.x);
        int newLeftmostX = (int)floor(newX);

	    //jobboldali ütközés
	    if(isInBlockSideways(newRightmostX, lowestY, uppermostY)) {
	          playerPosition.x = newRightmostX -  playerSize.x - 0.0001;
	          playerVelocity.x = 0;
	    }
	    //baloldali ütközés
	    else if(isInBlockSideways(newLeftmostX, lowestY, uppermostY)) {
	         playerPosition.x = newLeftmostX + 1.0001;
	         playerVelocity.x = 0;
	    }
	    else  playerPosition.x = newX;
	}
    //<\új x hely kiszámítása>
    //<új y hely kiszámítása>
    int leftmostX = (int)floor(playerPosition.x);
    int rightmostX = (int)floor(playerPosition.x +  playerSize.x);

    if(isFalling) {
        if(playerVelocity.y > -playerMaxVelocity.y)
            playerVelocity.y -= g * deltaTime;
        float newY =  playerPosition.y +  playerVelocity.y * deltaTime;

	    int newLowestY = (int)floor(newY);
	    int newUppermostY = (int)floor(newY +  playerSize.y);

	    //felülről ér hozzá
	    if(isInBlockPerpendicular(leftmostX, rightmostX, newLowestY)) {
             playerPosition.y = newLowestY + 1.0001;
             playerVelocity.y = 0;
             isFalling = 0;
        }
        //alulról ér hozzá
        else if(isInBlockPerpendicular(leftmostX, rightmostX, newUppermostY)) {
             playerPosition.y = (int)floor(newUppermostY) -  playerSize.y - 0.0001;
             playerVelocity.y *= -0.1;
        }
        else  playerPosition.y = newY;
    }
    else {
        //ha van alatta valami, akkor ugorhat
        if(isInBlockPerpendicular(leftmostX, rightmostX, lowestY - 1)) {
            if(isJumping) {
                 playerVelocity.y =  playerMaxVelocity.y;
                 isFalling = 1;
            }
        }
        //különben elkezd esni
        else  isFalling = 1;
    }
    //</új y hely kiszámítása>
    //<pontok felvétele>
    block* treasureToRemove;
    if(detectCollision((vector2D) {playerPosition.x, playerPosition.y}, (vector2D) {playerSize.x, playerSize.y}, treasure, &treasureToRemove)) {
        updateText(++points);
        *treasureToRemove = air;
    }
    //<\pontok felvétele>

    followPlayerWithCamera();
}

void shoot(mousePos mouse) {
    if(points > 0) {
        updateText(--points);
        createProjectile(playerPosition, playerSize, &playerVelocity, mouse);
        isFalling = true;
    }
}

///Itt kezeli az időlassítással kapcsolatos teendőket, levon a pontszámból.
int usedFrames = 0;
bool slowTime(bool isEnabled) {
    if(isEnabled && points > 0) {
        usedFrames++;
        if(usedFrames > 100) {
            usedFrames = 0;
            updateText(--points);
        }
        return true;
    }
    return false;
}

///A rajzoláshoz kell, átadja a megfelelő adatokat.
renderObject getPlayerRenderObject() {
    return (renderObject) {
        playerPosition,
        playerSize,
        playerColors,
        true
    };
}

void getPlayerStats(vector2D* maxVelocity) {
    *maxVelocity = playerMaxVelocity;
}
