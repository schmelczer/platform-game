#include "unicodePrint.h"


/** \brief A feliratok és a felület méretezéséhez használt arányszám.
 * A program az 1920x1080-as képernyőre készült, minden mérete ehhez képest értendő.
 * Csak a magassághoz kötni viszont nem szabad: álló tájolású telefonon a képernyő magas,
 * de keskeny, így a magasságból számolt betűk kilógnának a szélességből. A kisebbik
 * képernyőoldalhoz igazítva a szöveg akkora lesz, amekkorára a fekvő képernyőn is, tehát
 * a készülék elforgatása nem változtatja meg a méretét.
 */
float uiScale() {
    if(screen == NULL) return 1;
    return (float) smaller(screen -> w, screen -> h) / designHeight;
}

///Indításnál betölti a szükséges betűtípusokat.
void openFonts() {
    if(!TTF_WasInit()) TTF_Init();

    float scale = uiScale();
    fontSmaller = TTF_OpenFont("resources/Roboto-Thin.ttf", larger(1, (int)(40 * scale)));
    fontSmall = TTF_OpenFont("resources/Roboto-Thin.ttf", larger(1, (int)(48 * scale)));
    fontSmallBold = TTF_OpenFont("resources/Roboto-Light.ttf", larger(1, (int)(53 * scale)));
    fontLarge = TTF_OpenFont("resources/Roboto-Thin.ttf", larger(1, (int)(120 * scale)));
}

///Ha már nem kellenek, felszabadítja a betűtípusokat.
void freeFonts() {
    TTF_CloseFont(fontSmaller);
    TTF_CloseFont(fontSmall);
    TTF_CloseFont(fontSmallBold);
    TTF_CloseFont(fontLarge);
}

///Itt tárolom a használható karaktereket. Eldönti, hogy a bemenet köztük van-e.
bool isValidChar(wchar_t chr) {
        if(('A' <= chr && chr <= 'Z') || ('a' <= chr && chr <= 'z') || ('0' <= chr && chr <= '9')) return true;

        wchar_t validChars[] = L"öüóőúéáűíÖÜÓŐÚÉÁŰÍ|€ß$Łł¤<>#&@{}Đ[]đ+!%=×)(.- ";
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
wchar_t* concatUnicode(const wchar_t a[], const wchar_t b[]) {
    int aLength, bLength;
    for(aLength = 0; a[aLength] != 0; aLength++)
        ;
    for(bLength = 0; b[bLength] != 0; bLength++)
        ;

    wchar_t* newStr = (wchar_t*) malloc((aLength + bLength - 1 + 100) * sizeof(wchar_t));
    int i = 0, ai, bi;
    for(ai = 0; ai < aLength; ai++)
        newStr[i++] = a[ai];
    for(bi = 0; bi < bLength; bi++)
        newStr[i++] = b[bi];
    newStr[i] = 0;

    return newStr;
}

///A megjelenítendő szöveg közepét kell megadni x,y-ként. A szöveget Uint16*-ban, a betűtipust pedig a betűtipus enumból. Ugyan mindent x = 50-nel hívunk meg, ez később változhat, ezért hagyom meg paraméternek.
void unicodePrintCentered(wchar_t textToWrite[], int x, int y, fontType fontType) {
    char utf8[2048];
    wideToUtf8(textToWrite, utf8, sizeof(utf8));
    utf8PrintCentered(utf8, x, y, fontType);
}

///Ugyanaz, mint az unicodePrintCentered, csak utf-8-as bemenettel.
void utf8PrintCentered(const char* textToWrite, int x, int y, fontType fontType) {
    if(!isRenderingEnabled()) return;

    TTF_Font* currentFont = selectFont(fontType);
    SDL_FreeSurface(text);
    text = TTF_RenderUTF8_Blended(currentFont, textToWrite, (SDL_Color) {255, 255, 255});
    if(text == NULL) return;
    SDL_BlitSurface(text, NULL, screen, &(SDL_Rect){percentageToPx(x, true) - text -> w / 2, percentageToPx(y, false) - text -> h / 2, 0, 0});
}

int wideToUtf8(const wchar_t* source, char* destination, int destinationSize) {
    int written = 0;

    for(; *source != 0; source++) {
        unsigned int c = (unsigned int) *source;
        int needed = c < 0x80 ? 1 : c < 0x800 ? 2 : c < 0x10000 ? 3 : 4;
        if(written + needed >= destinationSize) break;

        switch(needed) {
            case 1:
                destination[written++] = c;
                break;
            case 2:
                destination[written++] = 0xC0 | (c >> 6);
                destination[written++] = 0x80 | (c & 0x3F);
                break;
            case 3:
                destination[written++] = 0xE0 | (c >> 12);
                destination[written++] = 0x80 | ((c >> 6) & 0x3F);
                destination[written++] = 0x80 | (c & 0x3F);
                break;
            default:
                destination[written++] = 0xF0 | (c >> 18);
                destination[written++] = 0x80 | ((c >> 12) & 0x3F);
                destination[written++] = 0x80 | ((c >> 6) & 0x3F);
                destination[written++] = 0x80 | (c & 0x3F);
        }
    }

    destination[written] = 0;
    return written;
}

int utf8ToWide(const char* source, wchar_t* destination, int destinationSize) {
    int written = 0;

    while(*source != 0 && written < destinationSize - 1) {
        unsigned char lead = (unsigned char) *source++;
        unsigned int c;
        int continuations;

        if(lead < 0x80) { c = lead; continuations = 0; }
        else if((lead & 0xE0) == 0xC0) { c = lead & 0x1F; continuations = 1; }
        else if((lead & 0xF0) == 0xE0) { c = lead & 0x0F; continuations = 2; }
        else if((lead & 0xF8) == 0xF0) { c = lead & 0x07; continuations = 3; }
        else continue;

        int i;
        for(i = 0; i < continuations; i++) {
            if((*source & 0xC0) != 0x80) break;
            c = (c << 6) | (*source++ & 0x3F);
        }

        destination[written++] = (wchar_t) c;
    }

    destination[written] = 0;
    return written;
}
