#include "unicodePrint.h"


///Indításnál betölti a szükséges betűtípusokat.
void openFonts() {
    SDL_EnableUNICODE(1);
    TTF_Init();
    fontSmaller = TTF_OpenFont("resources/Roboto-Thin.ttf", 40);
    fontSmall = TTF_OpenFont("resources/Roboto-Thin.ttf", 48);
    fontSmallBold = TTF_OpenFont("resources/Roboto-Light.ttf", 53);
    fontLarge = TTF_OpenFont("resources/Roboto-Thin.ttf", 120);
}

///Ha már nem kellenek, felszabadítja a betűtípusokat.
void freeFonts() {
    TTF_CloseFont(fontSmaller);
    TTF_CloseFont(fontSmall);
    TTF_CloseFont(fontSmallBold);
    TTF_CloseFont(fontLarge);
}

///Itt tárolom a használható karaktereket. Eldönti, hogy a bemenet köztük van-e.
bool isValidChar(Uint16 chr) {
        if(('A' <= chr && chr <= 'Z') || ('a' <= chr && chr <= 'z') || ('0' <= chr && chr <= '9')) return true;

        Uint16 validChars[] = L"öüóőúéáűíÖÜÓŐÚÉÁŰÍ|€ß$Łł¤<>#&@{}Đ[]đ+!%=×)(.- ";
        int i;
        for(i = 0; validChars[i] != 0; i++) if(chr == validChars[i])
            return true;

        return false;
}

///Az egyszerűbb kezelhetőség miatt kell. Visszadja az enumhoz tartozó ttf-et.
TTF_Font* selectFont(fontType fontType) {
    switch(fontType) {
        case tiny:
            return fontSmaller;
        case small:
            return fontSmall;
        case smallBold:
            return fontSmallBold;
        case large:
        default:
            return fontLarge;
    }
}

///A százalékot a képernyő mérete alapján átváltja pixelekre.
Sint16 percentageToPx(int percentage, bool isWidth) {
    return (int)((float)percentage/100 * (isWidth ? screen -> w: screen -> h));
}

/** \brief Uint16-ból álló stringet összefűz.
 * A két paramétert egy harmadik dinamikusan foglalt stringben fűzi össze.
 * \param string eleje
 * \param string vége
 * \return string konkatenált
 */
Uint16* concatUnicode(const Uint16 a[], const Uint16 b[]) {
    int aLength, bLength;
    for(aLength = 0; a[aLength] != 0; aLength++)
        ;
    for(bLength = 0; b[bLength] != 0; bLength++)
        ;

    Uint16* newStr = (Uint16*) malloc((aLength + bLength - 1 + 100) * sizeof(Uint16));
    int i = 0, ai, bi;
    for(ai = 0; ai < aLength; ai++)
        newStr[i++] = a[ai];
    for(bi = 0; bi < bLength; bi++)
        newStr[i++] = b[bi];
    newStr[i] = 0;

    return newStr;
}

///A megjelenítendő szöveg közepét kell megadni x,y-ként. A szöveget Uint16*-ban, a betűtipust pedig a betűtipus enumból. Ugyan mindent x = 50-nel hívunk meg, ez később változhat, ezért hagyom meg paraméternek.
void unicodePrintCentered(Uint16 textToWrite[], int x, int y, fontType fontType) {
    TTF_Font* currentFont = selectFont(fontType);
    SDL_FreeSurface(text);
    text = TTF_RenderUNICODE_Blended(currentFont, textToWrite, (SDL_Color) {255, 255, 255});
    SDL_BlitSurface(text, NULL, screen, &(SDL_Rect){percentageToPx(x, true) - text -> w / 2, percentageToPx(y, false) - text -> h / 2, 0, 0});
}
