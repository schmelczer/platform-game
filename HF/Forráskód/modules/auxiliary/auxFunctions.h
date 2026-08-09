#ifndef AUXFUNCTIONS_H_INCLUDED
#define AUXFUNCTIONS_H_INCLUDED

    #include "../global.h"

    inline float randomWithinBorders(float base, float variance) {
        int intVariance = (int)(1e4 * variance);
        return (1e4 * base + (rand() % (2 * intVariance + 1) - intVariance)) / 1e4;
    }

    inline int randomIntWithinBorders(int base, int variance) {
        return base + (rand() % (2 * variance + 1) - variance);
    }

    inline int randomPlusOrMinus() {
        return rand() % 2 ? 1 : -1;
    }


    inline int larger(int a, int b) {
        return a > b ? a : b;
    }

    inline int smaller(int a, int b) {
        return a < b ? a : b;
    }

    inline float largerF(float a, float b) {
        return a > b ? a : b;
    }

    inline float smallerF(float a, float b) {
        return a < b ? a : b;
    }


    inline void putInsideInterval(int min, int* a, int max) {
        *a = largerF(min, smallerF(*a, max));
    }

    inline void putInsideIntervalF(float min, float* a, float max) {
        *a = largerF(min, smallerF(*a, max));
    }


    inline float dist(int a, int b) {
        return sqrt(a*a + b*b);
    }

    inline float fDist(float a, float b) {
        return sqrt(a*a + b*b);
    }

    inline float square(float a) {
        return a*a;
    }

#endif // AUXFUNCTIONS_H_INCLUDED
