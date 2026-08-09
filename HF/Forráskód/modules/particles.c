#include "particles.h"

#define particleBaseSize 0.08           /**< Az eső részecskék (~hó) átlagos mérete [m]. */
#define particleSizeVariance 0.05       /**< Az eső részecskék (~hó) méretének átlagostól való eltérése [m]. */
#define particleBaseVelocity 0.0025      /**< Az eső részecskék (~hó) átlagos sebessége [m/ms]. */
#define particleVelocityVariance 0.0010 /**< Az eső részecskék (~hó) sebességének átlagostól való eltérése [m/ms]. */
#define particleColor (colors) { (color) {220, 220, 220, 255}, (color){220, 220, 220, 255}, (color) {220, 220, 220, 255} } /**< A részecskék színe. */

int particleMultiplier;                 /**< Egy oszlopban hány részecske van? */
particle** particleList = NULL;         /**< 2D-s tömb a részecskéknek, x koord-ban rendezett, y-ban nem.*/


///Mennyi részecske van egy négyzetméteren, ezt lehet itt beállítani.
void setParticleMultiplier(int a, bool isMenu) {
    particleMultiplier = isMenu ? a : getMapSize().y * a / 100;
}

///A makrókban beállított paraméterek alapján hoz létre egy részecskét, a pálya és a kamera közti z koorddal.
particle createRandomParticle(int baseY, bool isYRand) {
    return (particle) {
        (vector2D) {isYRand ? rand() % (baseY-1) : baseY, (float) (rand() % (cameraDeltaZDistanceEnd - 1) + 1)},
        randomWithinBorders(particleBaseSize, particleSizeVariance),
        randomWithinBorders(particleBaseVelocity, particleVelocityVariance)
    };
}

///Felszabadítja a részecskék tömbjét.
void freeParticleList() {
    if(particleList != NULL) {
        free(particleList[0]);
        free(particleList);
    }
}

///Csinál egy tömböt a részecskéknek és azt feltölti random részesckékkel.
void createParticleList() {
    freeParticleList();

    intVector2D mapSize = getMapSize();
    int x, i;
    particleList = (particle**) malloc(mapSize.x * sizeof(particle*));
    particleList[0] = (particle*) malloc(mapSize.x * particleMultiplier * sizeof(particle));

    for(x = 0; x < mapSize.x; x++) {
        particleList[x] = particleList[0] + x * particleMultiplier;
        for(i = 0; i < particleMultiplier; i++)
            particleList[x][i] = createRandomParticle(mapSize.y, true);
    }
}

///Egy adott részecske mozgását végzi el.
void moveParticle(particle* particle, int delta) {
    ///! x = y; y = z
    particle -> position.x -= particle -> velocity * delta;

    if(particle -> position.x <= 0.0)
        *particle = createRandomParticle(getMapSize().y, false);

}

///Lerajzolja és mozgatja a részecskéket. Mindezt csak egy oszlopon belül.
void drawParticles(int x, int minY, int maxY, int deltaTime) {
    if(0 <= x && x < getMapSize().x) {
        int i;
        for(i = 0; i < particleMultiplier; i++) {
            moveParticle(&particleList[x][i], deltaTime);

            if(minY < particleList[x][i].position.x && particleList[x][i].position.x < maxY) {

                bool sides[5] = {true, true, true, true, true};
                drawCuboid (
                    (renderObject) {
                        (vector3D) {x, particleList[x][i].position.x, particleList[x][i].position.y},
                        (vector3D) {particleList[x][i].size, particleList[x][i].size, particleList[x][i].size / 2},
                        particleColor, true
                    },
                    sides
                );

            }
        }
    }
}
