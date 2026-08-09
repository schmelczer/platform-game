#include "gfxPrimitives.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct target {
    SDL_Surface* surface;
    Uint32* pixels;
    int pitch, w, h;
    int rShift, gShift, bShift;
    Uint32 aMask;
    bool locked;
} target;

static bool renderingEnabled = true;    /**< Kikapcsolva minden rajzoló primitív azonnal visszatér. */

/** \brief Ki-be kapcsolja a rajzolást.
 * A behozott képkockákat a main.c "utolérő" ciklusa többször is végigszimulálja, de csak
 * az utolsót mutatja meg. A közbülső lépések alatt kikapcsolva a rajzolás nem dolgozik feleslegesen.
 */
void setRenderingEnabled(bool enabled) {
    renderingEnabled = enabled;
}

bool isRenderingEnabled() {
    return renderingEnabled;
}

static bool beginTarget(SDL_Surface* dst, target* t) {
    if(!renderingEnabled || dst == NULL || dst -> format -> BytesPerPixel != 4) return false;

    t -> surface = dst;
    t -> locked = SDL_MUSTLOCK(dst);
    if(t -> locked && SDL_LockSurface(dst) != 0) return false;

    t -> pixels = (Uint32*) dst -> pixels;
    t -> pitch = dst -> pitch / 4;
    t -> w = dst -> w;
    t -> h = dst -> h;
    t -> rShift = dst -> format -> Rshift;
    t -> gShift = dst -> format -> Gshift;
    t -> bShift = dst -> format -> Bshift;
    t -> aMask = dst -> format -> Amask;
    return true;
}

static void endTarget(target* t) {
    if(t -> locked) SDL_UnlockSurface(t -> surface);
}

static inline Uint32 packColor(const target* t, Uint8 r, Uint8 g, Uint8 b) {
    return t -> aMask | ((Uint32) r << t -> rShift) | ((Uint32) g << t -> gShift) | ((Uint32) b << t -> bShift);
}

static inline void blendPixel(const target* t, int x, int y, Uint8 r, Uint8 g, Uint8 b, int a) {
    if(a <= 0 || x < 0 || y < 0 || x >= t -> w || y >= t -> h) return;

    Uint32* p = t -> pixels + y * t -> pitch + x;

    if(a >= 255) {
        *p = packColor(t, r, g, b);
        return;
    }

    Uint32 c = *p;
    int ia = 255 - a;
    Uint32 dr = (c >> t -> rShift) & 0xFF;
    Uint32 dg = (c >> t -> gShift) & 0xFF;
    Uint32 db = (c >> t -> bShift) & 0xFF;

    *p = packColor(t,
        (Uint8) ((r * a + dr * ia + 127) / 255),
        (Uint8) ((g * a + dg * ia + 127) / 255),
        (Uint8) ((b * a + db * ia + 127) / 255));
}

static void blendHLine(const target* t, int xa, int xb, int y, Uint8 r, Uint8 g, Uint8 b, int a) {
    if(y < 0 || y >= t -> h || a <= 0) return;
    if(xa > xb) { int tmp = xa; xa = xb; xb = tmp; }
    if(xa < 0) xa = 0;
    if(xb > t -> w - 1) xb = t -> w - 1;
    if(xa > xb) return;

    Uint32* p = t -> pixels + y * t -> pitch + xa;
    int i;

    if(a >= 255) {
        Uint32 c = packColor(t, r, g, b);
        for(i = xa; i <= xb; i++) *p++ = c;
    }
    else {
        int ia = 255 - a;
        int ra = r * a, ga = g * a, ba = b * a;

        for(i = xa; i <= xb; i++, p++) {
            Uint32 c = *p;
            *p = packColor(t,
                (Uint8) ((ra + ((c >> t -> rShift) & 0xFF) * ia + 127) / 255),
                (Uint8) ((ga + ((c >> t -> gShift) & 0xFF) * ia + 127) / 255),
                (Uint8) ((ba + ((c >> t -> bShift) & 0xFF) * ia + 127) / 255));
        }
    }
}

static inline void unpackColor(Uint32 color, Uint8* r, Uint8* g, Uint8* b, Uint8* a) {
    *r = (color >> 24) & 0xFF;
    *g = (color >> 16) & 0xFF;
    *b = (color >> 8) & 0xFF;
    *a = color & 0xFF;
}

int boxRGBA(SDL_Surface* dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    target t;
    if(!beginTarget(dst, &t)) return -1;

    if(x1 > x2) { Sint16 tmp = x1; x1 = x2; x2 = tmp; }
    if(y1 > y2) { Sint16 tmp = y1; y1 = y2; y2 = tmp; }

    int yStart = y1 < 0 ? 0 : y1;
    int yEnd = y2 > t.h - 1 ? t.h - 1 : y2;

    int y;
    for(y = yStart; y <= yEnd; y++) blendHLine(&t, x1, x2, y, r, g, b, a);

    endTarget(&t);
    return 0;
}

int boxColor(SDL_Surface* dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color) {
    Uint8 r, g, b, a;
    unpackColor(color, &r, &g, &b, &a);
    return boxRGBA(dst, x1, y1, x2, y2, r, g, b, a);
}

static int lineRGBA(SDL_Surface* dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    target t;
    if(!beginTarget(dst, &t)) return -1;

    int dx = abs(x2 - x1), dy = -abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1, sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;
    int x = x1, y = y1;

    while(true) {
        blendPixel(&t, x, y, r, g, b, a);
        if(x == x2 && y == y2) break;
        int e2 = 2 * err;
        if(e2 >= dy) { err += dy; x += sx; }
        if(e2 <= dx) { err += dx; y += sy; }
    }

    endTarget(&t);
    return 0;
}

int lineColor(SDL_Surface* dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color) {
    Uint8 r, g, b, a;
    unpackColor(color, &r, &g, &b, &a);
    return lineRGBA(dst, x1, y1, x2, y2, r, g, b, a);
}

int filledCircleColor(SDL_Surface* dst, Sint16 x, Sint16 y, Sint16 rad, Uint32 color) {
    Uint8 r, g, b, a;
    unpackColor(color, &r, &g, &b, &a);

    target t;
    if(!beginTarget(dst, &t)) return -1;

    int dy;
    for(dy = -rad; dy <= rad; dy++) {
        int span = (int) floor(sqrt((double) rad * rad - (double) dy * dy));
        blendHLine(&t, x - span, x + span, y + dy, r, g, b, a);
    }

    endTarget(&t);
    return 0;
}

int aacircleColor(SDL_Surface* dst, Sint16 x, Sint16 y, Sint16 rad, Uint32 color) {
    Uint8 r, g, b, a;
    unpackColor(color, &r, &g, &b, &a);

    target t;
    if(!beginTarget(dst, &t)) return -1;

    int dx, dy;
    for(dy = -rad - 1; dy <= rad + 1; dy++)
        for(dx = -rad - 1; dx <= rad + 1; dx++) {
            double d = sqrt((double) dx * dx + (double) dy * dy);
            double coverage = 1.0 - fabs(d - rad);
            if(coverage > 0.0)
                blendPixel(&t, x + dx, y + dy, r, g, b, (int) (a * (coverage > 1.0 ? 1.0 : coverage)));
        }

    endTarget(&t);
    return 0;
}

int filledPolygonRGBA(SDL_Surface* dst, const float* vx, const float* vy, int n, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if(n < 3) return -1;

    target t;
    if(!beginTarget(dst, &t)) return -1;

    //A játék végig négyszögeket rajzol, azoknak bőven elég a veremben álló tömb;
    //képkockánként több tízezer malloc/free párt spórolunk vele.
    double inlineCrossings[16];
    double* crossings = inlineCrossings;
    if(n > (int)(sizeof(inlineCrossings) / sizeof(*inlineCrossings))) {
        crossings = (double*) malloc(n * sizeof(double));
        if(crossings == NULL) { endTarget(&t); return -1; }
    }

    double minY = vy[0], maxY = vy[0];
    int i;
    for(i = 1; i < n; i++) {
        if(vy[i] < minY) minY = vy[i];
        if(vy[i] > maxY) maxY = vy[i];
    }

    //A vágás még a törtszámokon megtörténik, így a kerekítés biztosan nem lóg ki az int-ből
    if(minY < 0) minY = 0;
    if(maxY > t.h - 1) maxY = t.h - 1;
    if(minY > maxY) { if(crossings != inlineCrossings) free(crossings); endTarget(&t); return 0; }

    int yStart = (int) ceil(minY);
    int yEnd = (int) floor(maxY);

    int y;
    for(y = yStart; y <= yEnd; y++) {
        int count = 0;
        for(i = 0; i < n; i++) {
            int j = (i + 1) % n;
            double y1 = vy[i], y2 = vy[j], x1 = vx[i], x2 = vx[j];
            if(y1 > y2) { double tmp = y1; y1 = y2; y2 = tmp; tmp = x1; x1 = x2; x2 = tmp; }

            //Félig nyílt intervallum, hogy a két él közös csúcsát pontosan egyszer számoljuk
            if(y >= y1 && y < y2)
                crossings[count++] = x1 + ((y - y1) * (x2 - x1)) / (y2 - y1);
        }

        if(count < 2) continue;

        //Néhány elemre a beszúrásos rendezés gyorsabb, mint a qsort függvénypointeres hívogatása
        int k;
        for(i = 1; i < count; i++) {
            double key = crossings[i];
            for(k = i - 1; k >= 0 && crossings[k] > key; k--) crossings[k + 1] = crossings[k];
            crossings[k + 1] = key;
        }

        for(i = 0; i + 1 < count; i += 2) {
            double left = crossings[i], right = crossings[i + 1];
            if(left < 0) left = 0;
            if(right > t.w - 1) right = t.w - 1;
            if(left > right) continue;

            int xa = (int) ceil(left), xb = (int) floor(right);
            if(xa <= xb) blendHLine(&t, xa, xb, y, r, g, b, a);
        }
    }

    if(crossings != inlineCrossings) free(crossings);
    endTarget(&t);
    return 0;
}

static void aaLine(const target* t, double x0, double y0, double x1, double y1, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    bool steep = fabs(y1 - y0) > fabs(x1 - x0);
    double tmp;
    if(steep) { tmp = x0; x0 = y0; y0 = tmp; tmp = x1; x1 = y1; y1 = tmp; }
    if(x0 > x1) { tmp = x0; x0 = x1; x1 = tmp; tmp = y0; y0 = y1; y1 = tmp; }

    double dx = x1 - x0;
    double gradient = dx == 0.0 ? 0.0 : (y1 - y0) / dx;

    int xStart = (int) floor(x0 + 0.5), xEnd = (int) floor(x1 + 0.5);
    double intersection = y0 + gradient * (xStart - x0);

    int x;
    for(x = xStart; x <= xEnd; x++) {
        int base = (int) floor(intersection);
        double frac = intersection - base;

        if(steep) {
            blendPixel(t, base, x, r, g, b, (int) (a * (1.0 - frac)));
            blendPixel(t, base + 1, x, r, g, b, (int) (a * frac));
        }
        else {
            blendPixel(t, x, base, r, g, b, (int) (a * (1.0 - frac)));
            blendPixel(t, x, base + 1, r, g, b, (int) (a * frac));
        }

        intersection += gradient;
    }
}

int aapolygonRGBA(SDL_Surface* dst, const float* vx, const float* vy, int n, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if(n < 2) return -1;

    target t;
    if(!beginTarget(dst, &t)) return -1;

    int i;
    for(i = 0; i < n; i++) {
        int j = (i + 1) % n;
        aaLine(&t, vx[i], vy[i], vx[j], vy[j], r, g, b, a);
    }

    endTarget(&t);
    return 0;
}
