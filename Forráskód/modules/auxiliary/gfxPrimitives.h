#ifndef GFXPRIMITIVES_H_INCLUDED
#define GFXPRIMITIVES_H_INCLUDED

    #include <SDL.h>
    #include <stdbool.h>

    void setRenderingEnabled(bool enabled);
    bool isRenderingEnabled();

    int boxColor(SDL_Surface* dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color);
    int boxRGBA(SDL_Surface* dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    int lineColor(SDL_Surface* dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color);
    int filledCircleColor(SDL_Surface* dst, Sint16 x, Sint16 y, Sint16 rad, Uint32 color);
    int aacircleColor(SDL_Surface* dst, Sint16 x, Sint16 y, Sint16 rad, Uint32 color);
    int filledPolygonRGBA(SDL_Surface* dst, const float* vx, const float* vy, int n, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    int aapolygonRGBA(SDL_Surface* dst, const float* vx, const float* vy, int n, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

#endif // GFXPRIMITIVES_H_INCLUDED
