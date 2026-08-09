#include "global.h"
#include "saves.h"

float g;

SDL_Surface* screen;
TTF_Font* fontSmaller;
TTF_Font* fontSmall;
TTF_Font* fontSmallBold;
TTF_Font* fontLarge;
SDL_Surface* text;
unsigned char* keys;

bool existingSaves[maxNOSaves];
