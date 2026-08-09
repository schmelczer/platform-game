#ifndef UNICODEPRINT_H_INCLUDED
#define UNICODEPRINT_H_INCLUDED

    #include "../global.h"

    typedef enum fontType {
        tiny, small, smallBold, large
    } fontType;

    void openFonts();
    void freeFonts();
    bool isValidChar(Uint16 chr);
    Uint16* concatUnicode(const Uint16 a[], const Uint16 b[]);
    void unicodePrintCentered(Uint16 textToWrite[], int x, int y, fontType fontType);

#endif // UNICODEPRINT_H_INCLUDED
