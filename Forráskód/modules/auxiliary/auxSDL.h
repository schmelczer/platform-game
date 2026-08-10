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

    /** \brief Mobilon ennél is elmehetünk, mert ott a vászon css-ben is kicsi.
     * Egy telefon képernyője css-pixelben nagyjából ötöde egy asztali ablakénak, így a
     * kétszeres arány még mindig kevesebb rajzolandó pixelt jelent, mint amennyivel a
     * gépen számolunk. Cserébe a kicsi, sűrű kijelzőn ez látszik meg a legjobban.
     */
    #define maxMobileDevicePixelRatio 2.0

    /** \brief Eddig a css-pixelben mért rövidebb képernyőoldalig tekintünk egy eszközt mobilnak.
     * A mutatóeszköz mellé azért kell a méret is, mert egy nagy, ujjal kezelt panel (például
     * egy kihelyezett érintőképernyő) is az ujjat mondja fő mutatóeszköznek, a duplázás
     * viszont ott már ugyanolyan drága lenne, mint az asztali gépen. A határ a legnagyobb
     * kézbe fogható eszköz rövidebb oldala: efölött már nem telefonról vagy táblagépről van szó.
     */
    #define mobileScreenLimit 1024

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
