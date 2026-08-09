#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

    #include "global.h"

    #include "floaters.h"
    #include "player.h"
    #include "map.h"
    #include "particles.h"

    #define cameraDeltaZDistanceStart 25    /**<dolly start táv [m]*/
    #define cameraDeltaZDistanceEnd 14      /**<dolly end táv [m]*/
    #define cameraDollyTime 3250            /**<[ms]*/

    void freeRenderMap();
    void followPlayerWithCamera();
    void menuCamera(int* getColumns, int* getRows);
    int getColumns();
    bool cameraDollyZ(int deltaTime, bool restart);
    intVector2D get2DPoint(vector3D point);
    void drawCuboid(renderObject object, bool sides[]);
    void drawMap(int deltaTime);

#endif // CAMERA_H_INCLUDED
