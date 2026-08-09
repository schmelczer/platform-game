#ifndef AUXSDL_H_INCLUDED
#define AUXSDL_H_INCLUDED

    #include "../global.h"
    #include "unicodePrint.h"

    #define mouseSize 20    /**< Az egérmutató négyzetnek az oldalhosszusága. */

    #define defaultWindowWidth 1920     /**< Az ablak alapértelmezett szélessége. */
    #define defaultWindowHeight 1080    /**< Az ablak alapértelmezett magassága. */

    /** \brief Böngészőben legfeljebb ennyi rajzfelület-pixel jusson egy css-pixelre.
     * A teljes képet a processzor rajzolja, így a költsége a pixelek számával nő. Egy retina
     * kijelzőn a natív felbontás négyszer annyi pixelt jelentene; helyette css-méretben rajzolunk,
     * a vásznat pedig a böngésző nagyítja fel, ami már ingyen van.
     */
    #define maxDevicePixelRatio 1.0

    typedef struct mousePos {
        Uint16 x, y;
    } mousePos;

    void openWindow();
    void handleResize();
    void presentFrame();
    void closeWindow();
    void setMousePosition(Uint16 x, Uint16 y);
    mousePos getMousePosition();
    void drawMouse(bool isBW);
    bool isMouseOverRect(int xCenter, int yCenter, int xSize, int ySize);

#endif // AUXSDL_H_INCLUDED
