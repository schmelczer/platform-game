#include "auxSDL.h"
#include "../camera.h"

#ifdef __EMSCRIPTEN__
    #include <emscripten/html5.h>
#else
    #include <unistd.h>
#endif

mousePos mouse; /**< Az egér koordinátáit tárolja, így egyszerűbb, mint közvetlen az eventek figyelése. */

static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_Texture* frameTexture;
static int textureWidth, textureHeight;

#ifdef __EMSCRIPTEN__
///A böngésző pixelarányát adja vissza, a szoftveres rajzolás miatt megszabott plafonig.
static double drawingPixelRatio() {
    double ratio = emscripten_get_device_pixel_ratio();
    return ratio > maxDevicePixelRatio ? maxDevicePixelRatio : ratio;
}
#endif

static void moveToResourceDirectory() {
#ifndef __EMSCRIPTEN__
    char* base = SDL_GetBasePath();
    if(base != NULL) {
        if(chdir(base) != 0) fprintf(stderr, "Nem sikerult a munkakonyvtar valtas!\n");
        SDL_free(base);
    }
#endif
}

/** \brief Gondoskodik róla, hogy a textúra legalább a kért méretű legyen.
 * Böngészőben a textúra eldobása átméretezéskor elveszejti a webgl kontextust, ezért
 * a textúrát egyszer, jó bőre foglaljuk, és utána már csak egy részét használjuk.
 */
static void ensureTexture(int width, int height) {
    if(frameTexture != NULL && width <= textureWidth && height <= textureHeight) return;

    if(width > textureWidth) textureWidth = width;
    if(height > textureHeight) textureHeight = height;

    SDL_DestroyTexture(frameTexture);
    frameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, textureWidth, textureHeight);

    if(frameTexture == NULL) {
        fprintf(stderr, "Nem sikerult letrehozni a texturat: %s\n", SDL_GetError());
        exit(1);
    }
}

/** \brief A megadott méretű rajzfelületet hozza létre a régi helyére.
 * A rajzfelület mindig pontosan akkora, mint a megjelenítő felülete, így nincs se
 * fekete csík, se átméretezés.
 */
static void createDrawingSurface(int width, int height) {
    if(width < 1) width = 1;
    if(height < 1) height = 1;

    ensureTexture(width, height);

    SDL_FreeSurface(screen);
    screen = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);

    if(screen == NULL) {
        fprintf(stderr, "Nem sikerult letrehozni a rajzfeluletet: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_RenderSetLogicalSize(renderer, width, height);
}

/** \brief Az ablak méretének változását követi le, képkockánként meghívva.
 * Böngészőben a vászon css-méretéhez igazítja az ablakot, majd ha kell, új rajzfelületet
 * készít, a betűtípusokat a magassághoz igazítja, és a kamerával újraszámoltatja,
 * hogy mennyi látszik a pályából.
 */
void handleResize() {
#ifdef __EMSCRIPTEN__
    double cssWidth, cssHeight;
    if(emscripten_get_element_css_size("#canvas", &cssWidth, &cssHeight) == EMSCRIPTEN_RESULT_SUCCESS) {
        double ratio = drawingPixelRatio();
        int wantedWidth = (int)(cssWidth * ratio);
        int wantedHeight = (int)(cssHeight * ratio);

        int currentWidth, currentHeight;
        SDL_GetWindowSize(window, &currentWidth, &currentHeight);

        if(wantedWidth > 0 && wantedHeight > 0 && (wantedWidth != currentWidth || wantedHeight != currentHeight))
            SDL_SetWindowSize(window, wantedWidth, wantedHeight);
    }
#endif

    int outputWidth, outputHeight;
    SDL_GetRendererOutputSize(renderer, &outputWidth, &outputHeight);

    if(screen != NULL && screen -> w == outputWidth && screen -> h == outputHeight) return;

    //a betűméret a kisebbik képernyőoldalhoz igazodik, csak annak változásakor kell újratölteni
    bool scaleChanged = screen == NULL ||
        smaller(screen -> w, screen -> h) != smaller(outputWidth, outputHeight);
    createDrawingSurface(outputWidth, outputHeight);

    if(scaleChanged) {
        freeFonts();
        openFonts();
    }

    refreshCameraForScreen();
}

/** \brief Létrehozza a játék futtatásához szükséges környezetet.
 * Létrehoz egy ablakot a hozzá tartozó rajzfelülettel, illetve
 * meghív pár, az inicializáláshoz szükséges függvényt.
 */
void openWindow() {
    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Nem sikerult inicializalni az SDL-t: %s\n", SDL_GetError());
        exit(1);
    }

    moveToResourceDirectory();

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    int windowWidth = defaultWindowWidth;
    int windowHeight = defaultWindowHeight;
    Uint32 windowFlags = 0;

#ifdef __EMSCRIPTEN__
    double cssWidth, cssHeight;
    if(emscripten_get_element_css_size("#canvas", &cssWidth, &cssHeight) == EMSCRIPTEN_RESULT_SUCCESS) {
        double ratio = drawingPixelRatio();
        windowWidth = (int) (cssWidth * ratio);
        windowHeight = (int) (cssHeight * ratio);
    }
#else
    windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

    SDL_Rect usableBounds;
    if(SDL_GetDisplayUsableBounds(0, &usableBounds) == 0) {
        float fit = smallerF((float) usableBounds.w / defaultWindowWidth, (float) usableBounds.h / defaultWindowHeight);
        if(fit < 1.0f) {
            windowWidth = (int) (defaultWindowWidth * fit);
            windowHeight = (int) (defaultWindowHeight * fit);
        }
    }
#endif

    const char* forcedSize = SDL_getenv("GAME_WINDOW");
    if(forcedSize != NULL) sscanf(forcedSize, "%dx%d", &windowWidth, &windowHeight);

    window = SDL_CreateWindow("Platform játék",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        windowWidth, windowHeight,
        windowFlags);

    if(window == NULL) {
        fprintf(stderr, "Nem sikerult megnyitni az ablakot: %s\n", SDL_GetError());
        exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(renderer == NULL) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if(renderer == NULL) {
        fprintf(stderr, "Nem sikerult letrehozni a renderert: %s\n", SDL_GetError());
        exit(1);
    }

#ifdef __EMSCRIPTEN__
    int screenPixelWidth, screenPixelHeight;
    emscripten_get_screen_size(&screenPixelWidth, &screenPixelHeight);
    double screenRatio = drawingPixelRatio();
    textureWidth = (int)(screenPixelWidth * screenRatio);
    textureHeight = (int)(screenPixelHeight * screenRatio);
#else
    SDL_DisplayMode desktop;
    if(SDL_GetDesktopDisplayMode(0, &desktop) == 0) {
        textureWidth = desktop.w;
        textureHeight = desktop.h;
    }
#endif

    int outputWidth, outputHeight;
    SDL_GetRendererOutputSize(renderer, &outputWidth, &outputHeight);
    createDrawingSurface(outputWidth, outputHeight);

    SDL_ShowCursor(SDL_DISABLE);
    openFonts();
}

void presentFrame() {
    SDL_Rect used = {0, 0, screen -> w, screen -> h};
    SDL_UpdateTexture(frameTexture, &used, screen -> pixels, screen -> pitch);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, frameTexture, &used, NULL);
    SDL_RenderPresent(renderer);
}

void closeWindow() {
    SDL_FreeSurface(screen);
    SDL_DestroyTexture(frameTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

///Frissití az egér pozicióját.
void setMousePosition(Uint16 x, Uint16 y) {
    mouse.x = x;
    mouse.y = y;
}

///Visszaadja az egérpoziciót.
mousePos getMousePosition() {
    return mouse;
}

///Lerajzolja a képernyőre az egeret, erre azért van szükség, mert az SDL csak fekete-fehér egérmutatót kezel.
void drawMouse(bool isBW) {
    Sint16 size = larger(4, (int)(mouseSize * uiScale()));
    Sint16 x1 = mouse.x - size / 2;
    Sint16 x2 = mouse.x + size / 2;
    Sint16 y1 = mouse.y - size / 2;
    Sint16 y2 = mouse.y + size / 2;
    boxColor(screen, x1, y1, x2, y2, isBW ? 0x7F7F7FFF : 0xFFFF00FF);
}

///Egy téglalap felett van-e az egérmutató közepe?
bool isMouseOverRect(int xCenter, int yCenter, int xSize, int ySize) {
    return
        screen -> w / 100.0 * (xCenter - xSize) < mouse.x && mouse.x < screen -> w / 100.0 * (xCenter + xSize) &&
        screen -> h / 100.0 * (yCenter - ySize) < mouse.y && mouse.y < screen -> h / 100.0 * (yCenter + ySize);
}
