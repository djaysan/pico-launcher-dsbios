// Host-side dump of EVERY palette a seed can reach, as one table.
//
// dump_scheme answers "what does this one seed give me". This answers "what is
// the whole reachable space", so a browser can move the seed live without any
// of the CAM16/HCT maths being reimplemented outside this tree.
//
// It exploits how CorePalette is built (see palettes/core.cpp): with a
// non-content palette, only primary's chroma follows the seed - secondary is
// fixed at 16, tertiary 24, neutral 4, neutral_variant 8. So the table splits:
//
//   bg[mode][hue]           neutral + neutral_variant roles   (hue only)
//   acc[mode][hue]          secondary + tertiary roles        (hue only)
//   pri[mode][chroma][hue]  primary roles                     (hue AND chroma)
//
// which is also exactly the line a SECOND seed would be cut along: bg from one
// seed, acc+pri from another. Compose the two halves and you have previewed a
// two-seed theme before a line of launcher code is written.
//
// Keep in sync with themes/material/MaterialColorSchemeFactory.cpp.
// Build/run: see run.sh in this folder.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include "material/palettes/core.h"
#include "material/scheme/scheme.h"
#include "material/cam/cam.h"
#include "material/cam/hct.h"
#include "material/utils/utils.h"

using namespace material_color_utilities;

static const int HUES = 360;
static const double CHROMAS[] = { 48.0, 72.0, 96.0, 120.0 };
static const int NCHROMA = 4;
// light, dark, dark+pureBlack - the three MaterialColorSchemeFactory branches
static const char* MODES[] = { "light", "dark", "black" };

struct Tones { double background, surface, card; bool dark; };
static Tones modeTones(int m)
{
    if (m == 0) return { 0, 0, 0, false };            // light ignores these
    if (m == 1) return { 10.0, 24.0, 22.0, true };
    return { 0.0, 16.0, 12.0, true };                 // pureBlack
}

static void putHex(Argb c) { std::printf("%02X%02X%02X", RedFromInt(c), GreenFromInt(c), BlueFromInt(c)); }

// The seven roles that come off neutral / neutral_variant.
static void emitBg(CorePalette& core, int m)
{
    Tones t = modeTones(m);
    Scheme s = t.dark ? MaterialDarkColorSchemeFromPalette(core)
                      : MaterialLightColorSchemeFromPalette(core);
    putHex(t.dark ? core.neutral().get(t.background) : s.inverse_on_surface);  // inverseOnSurface
    putHex(core.neutral().get(t.dark ? t.surface : 98.0));                     // surfaceBright
    putHex(core.neutral().get(t.dark ? t.card : 90.0));                        // surfaceContainerHighest
    putHex(core.neutral().get(t.dark ? 70.0 : 30.0));                          // scrim
    putHex(s.on_surface);
    putHex(s.on_surface_variant);
    putHex(s.outline);
}

// The seven roles that come off secondary / tertiary.
static void emitAcc(CorePalette& core, int m)
{
    bool dark = m != 0;
    Scheme s = dark ? MaterialDarkColorSchemeFromPalette(core)
                    : MaterialLightColorSchemeFromPalette(core);
    putHex(s.secondary_container);
    putHex(s.on_secondary_container);
    putHex(core.secondary().get(dark ? 42.0 : 78.0));                          // mainIconBg
    putHex(s.tertiary);
    putHex(s.on_tertiary);
    putHex(s.tertiary_container);
    putHex(s.on_tertiary_container);
}

static void emitPri(CorePalette& core, int m)
{
    bool dark = m != 0;
    Scheme s = dark ? MaterialDarkColorSchemeFromPalette(core)
                    : MaterialLightColorSchemeFromPalette(core);
    putHex(s.primary);
    putHex(s.on_primary);
}

// A real sRGB seed for (hue, chroma): the tone whose round trip through CAM16
// lands closest to what was asked for. Reported so the page can export a
// primaryColor that actually reproduces the preview.
struct SeedPick { Argb argb; double hueErr, chromaErr; };
static SeedPick pickSeed(double hue, double chroma)
{
    SeedPick best { 0, 1e9, 1e9 };
    double bestScore = 1e18;
    for (double tone = 20; tone <= 90; tone += 1.0)
    {
        Hct hct(hue, chroma, tone);
        Argb argb = hct.ToInt();
        Cam cam = CamFromInt(argb);
        double dh = std::fabs(cam.hue - hue);
        if (dh > 180) dh = 360 - dh;
        double dc = std::fabs(cam.chroma - chroma);
        // hue is what the whole palette hangs off, so weight it hard; chroma
        // only shifts primary, and is clamped by sRGB anyway
        double score = dh * 40 + dc;
        if (score < bestScore) { bestScore = score; best = { argb, dh, dc }; }
    }
    return best;
}

int main()
{
    std::printf("{\"hues\":%d,\"chromas\":[48,72,96,120],\"modes\":[\"light\",\"dark\",\"black\"]", HUES);

    for (int m = 0; m < 3; m++)
    {
        std::printf(",\"bg_%s\":\"", MODES[m]);
        for (int h = 0; h < HUES; h++) { CorePalette c = CorePalette::Of((double)h, 48.0); emitBg(c, m); }
        std::printf("\"");
        std::printf(",\"acc_%s\":\"", MODES[m]);
        for (int h = 0; h < HUES; h++) { CorePalette c = CorePalette::Of((double)h, 48.0); emitAcc(c, m); }
        std::printf("\"");
        for (int ci = 0; ci < NCHROMA; ci++)
        {
            std::printf(",\"pri_%s_%d\":\"", MODES[m], ci);
            for (int h = 0; h < HUES; h++)
            {
                CorePalette c = CorePalette::Of((double)h, CHROMAS[ci]);
                emitPri(c, m);
            }
            std::printf("\"");
        }
    }

    // seed swatches + how far each cell's chroma is actually reachable in sRGB
    double worstHue = 0;
    std::printf(",\"seeds\":[");
    for (int ci = 0; ci < NCHROMA; ci++)
    {
        std::printf("%s\"", ci ? "," : "");
        for (int h = 0; h < HUES; h++)
        {
            SeedPick p = pickSeed((double)h, CHROMAS[ci]);
            if (p.hueErr > worstHue) worstHue = p.hueErr;
            putHex(p.argb);
        }
        std::printf("\"");
    }
    std::printf("]");

    std::printf(",\"maxChromaIdx\":[");
    for (int h = 0; h < HUES; h++)
    {
        int top = 0;
        for (int ci = 0; ci < NCHROMA; ci++)
        {
            SeedPick p = pickSeed((double)h, CHROMAS[ci]);
            if (p.chromaErr <= 2.0) top = ci;
        }
        std::printf("%s%d", h ? "," : "", top);
    }
    std::printf("]");

    std::printf(",\"worstSeedHueError\":%.4f}\n", worstHue);
    return 0;
}
