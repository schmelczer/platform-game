#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <wchar.h>
#include <locale.h>

#include <SDL.h>
#include <SDL_ttf.h>
#include "auxiliary/gfxPrimitives.h"

#include "auxiliary/auxFunctions.h"

#ifndef GLOBAL_H_INCLUDED
#define GLOBAL_H_INCLUDED

#define FPS (int) 50
#define designHeight 1080   /**< Ehhez a magassághoz vannak méretezve a betűtípusok. */

extern float g;

///<Globális változók>
extern SDL_Surface* screen;
extern TTF_Font* fontSmaller;
extern TTF_Font* fontSmall;
extern TTF_Font* fontSmallBold;
extern TTF_Font* fontLarge;
extern SDL_Surface* text;
extern unsigned char* keys;
///</Globális változók>


///<Vektorok>
typedef struct vector2D {
    //1 egység == 1m
    float x, y;
} vector2D;

typedef struct intVector2D {
    int x, y;
} intVector2D;

typedef struct vector3D {
    //1 egység == 1m
    float x, y, z;
} vector3D;
///</Vektorok>


///<Színek>
typedef struct color {
    unsigned char r, g, b, a;
} color;

typedef struct colors {
    color light, normal, dark;
} colors;
///</Színek>


typedef enum block {
    air, solid, invisible, treasure
} block;


typedef struct renderObject {
    vector3D position;
    vector3D size;
    colors colors;
    bool doesExist;
} renderObject;

#endif // GLOBAL_H_INCLUDED
