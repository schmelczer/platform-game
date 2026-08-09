#include "color.h"

#define baseHue 30                      /**< A kockák átlagos színezetsége. */
#define hueVariance 20                  /**< A kockák színezetségének átlagostól vlaó eltérése. */
#define baseSaturation 0.75             /**< A kockák átlagos telítettsége. */
#define baseLightness 0.6               /**< A kockák átlagos fényessége. */
#define lightnessStep 0.08              /**< A kockák oldalainak háromféle fényesség értéke van, ezzel szimuláljuk a fény-árnyékot. */


///Ez a függvény majdnem teljes egészében erről az oldalról származik: https://www.programmingalgorithms.com/algorithm/hsl-to-rgb?lang=C
static float hueToRgb(float v1, float v2, float vH) {
	if (vH < 0) vH += 1;
	if (vH > 1) vH -= 1;
	if ((6 * vH) < 1) return (v1 + (v2 - v1) * 6 * vH);
	if ((2 * vH) < 1) return v2;
	if ((3 * vH) < 2) return (v1 + (v2 - v1) * ((2.0f / 3) - vH) * 6);
	return v1;
}

///Ez a függvény majdnem teljes egészében erről az oldalról származik: https://www.programmingalgorithms.com/algorithm/hsl-to-rgb?lang=C
static color hslToRgb(int h, float s, float l) {
	color rgb;
	if (s == 0) rgb.r = rgb.g = rgb.b = (unsigned char)(l * 255);
	else {
		float v1, v2;
		float hue = (float)h / 360;

		v2 = (l < 0.5) ? (l * (1 + s)) : ((l + s) - (l * s));
		v1 = 2 * l - v2;

		rgb.r = (unsigned char)(255 * hueToRgb(v1, v2, hue + (1.0 / 3)));
		rgb.g = (unsigned char)(255 * hueToRgb(v1, v2, hue));
		rgb.b = (unsigned char)(255 * hueToRgb(v1, v2, hue - (1.0 / 3)));
		rgb.a = 255;
	}
	return rgb;
}

/** \brief Véletlenszerűre színez egy kockát.
 * \return színek
 * Az alapszín, és hueVariance alapján meghatároz
 * véletlenszerűen egy színezetséget, és az ahhoz a színezetséghez tartozó világosabb és sötétebb
 * árnyalatú színeket is, majd ezeket RGBA-ba kódolja.
 * Egy random hue-val készített paletta sokkal jobban néz ki, mint egy random RGB-vel készített.
 */
colors createBlockColor() {
    int hue = randomIntWithinBorders(baseHue, hueVariance);

    if(hue < 0) hue += 360;

    return (colors) {
        hslToRgb(hue, baseSaturation, baseLightness + lightnessStep),
        hslToRgb(hue, baseSaturation, baseLightness),
        hslToRgb(hue, baseSaturation, baseLightness - lightnessStep)
    };
}
