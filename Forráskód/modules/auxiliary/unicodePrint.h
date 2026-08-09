#ifndef UNICODEPRINT_H_INCLUDED
#define UNICODEPRINT_H_INCLUDED

    #include "../global.h"

    typedef enum fontType {
        tiny, small, smallBold, large
    } fontType;

    void openFonts();
    void freeFonts();
    bool isValidChar(wchar_t chr);
    wchar_t* concatUnicode(const wchar_t a[], const wchar_t b[]);
    void unicodePrintCentered(wchar_t textToWrite[], int x, int y, fontType fontType);
    void utf8PrintCentered(const char* textToWrite, int x, int y, fontType fontType);
    int wideToUtf8(const wchar_t* source, char* destination, int destinationSize);
    int utf8ToWide(const char* source, wchar_t* destination, int destinationSize);

#endif // UNICODEPRINT_H_INCLUDED
