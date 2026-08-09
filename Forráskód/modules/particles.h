#ifndef PARTICLES_H_INCLUDED
#define PARTICLES_H_INCLUDED

    #include "global.h"
    #include "camera.h"
    #include "map.h"

    typedef struct particle {
        //y, z
        vector2D position;
        //mert kockák
        float size;
        float velocity;
    } particle;

    void setParticleMultiplier(int a, bool isMenu);
    void createParticleList();
    void freeParticleList();
    void drawParticles(int x, int minY, int maxY, int deltaTime);

#endif // PARTICLES_H_INCLUDED
