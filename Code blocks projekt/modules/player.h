#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

    #include "global.h"
    #include "map.h"
    #include "auxiliary/auxSDL.h"

    void setPlayerGravity(int gravity);

    void setDie(bool newValue);
    bool isAlive();

    int getPoints();
    void setPoints(int newAmount);

    void updateText(int newPoints);

    void shoot(mousePos mouse);
    bool slowTime(bool isEnabled);
    void getPlayerStats(vector2D* maxVelocity);
    void setPlayerStartingPosition(int x, int y);
    void movePlayer(unsigned char* keys, int deltaTime);
    renderObject getPlayerRenderObject();
    void drawPointsText();

#endif // PLAYER_H_INCLUDED
