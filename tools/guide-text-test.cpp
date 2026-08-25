// Host self-check for the guide reader's text rules, run against real guides.
//
//   g++ -std=c++20 -I arm9/source tools/guide-text-test.cpp -o /tmp/gtest && \
//   /tmp/gtest "path/to/some guide.txt"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include "romBrowser/viewModels/GuideText.h"

using namespace GuideText;

static unsigned decode1(const char* s, unsigned* consumed)
{
    unsigned pos = 0;
    unsigned cp = Decode((const unsigned char*)s, (unsigned)strlen(s), pos);
    if (consumed) *consumed = pos;
    return cp;
}

static bool prose(const char* s)
{
    return IsFlowedProse((const unsigned char*)s, (unsigned)strlen(s));
}

int main(int argc, char** argv)
{
    // --- UTF-8 ---------------------------------------------------------
    unsigned used;
    assert(decode1("A", &used) == 'A' && used == 1);
    assert(decode1("\xE2\x80\x99", &used) == 0x2019 && used == 3);   // right quote
    assert(decode1("\xC3\xA9", &used) == 0xE9 && used == 2);         // e acute
    assert(decode1("\xE2\x86\x92", &used) == 0x2192 && used == 3);   // arrow
    // a lone high byte is Latin-1, not a crash: emit it and move on by one
    assert(decode1("\xE9", &used) == 0xE9 && used == 1);
    // a truncated sequence must still advance, or the reader would spin
    assert(decode1("\xE2\x80", &used) == 0xE2 && used == 1);
    { const unsigned char b[] = {0xE2}; unsigned p = 0; Decode(b, 1, p); assert(p == 1); }

    // --- what flows, and what must not ---------------------------------
    assert(prose("encounter, Zelda's handmaiden Impa apprised a lad named Link of the situation,"));
    assert(prose("finally gained the power to face Ganon head-on. In the process, he introduced"));
    assert(!prose(""));                                   // blank ends a paragraph
    assert(!prose("Zelda II: The Adventure of Link"));     // heading, too short
    assert(!prose("   /\\                /\\"));            // indented art
    assert(!prose("|  Item      | Cost  | Where to find it        |"));  // table
    assert(!prose("=============================================================="));
    assert(prose(" a one space margin is a style, so this long sentence still flows on"));
    assert(!prose("      six spaces in is a block quote and must keep its own line breaks"));
    assert(!prose("Name        Cost        Location        Notes for later"));  // columns

    // --- against a real guide, if one was given -------------------------
    if (argc > 1)
    {
        std::ifstream f(argv[1], std::ios::binary);
        if (!f) { printf("could not open %s\n", argv[1]); return 1; }
        std::string all((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        unsigned flowed = 0, total = 0, artKept = 0;
        size_t start = 0;
        while (start < all.size())
        {
            size_t end = all.find('\n', start);
            if (end == std::string::npos) end = all.size();
            unsigned len = (unsigned)(end - start);
            while (len > 0 && all[start + len - 1] == '\r') len--;
            bool p = IsFlowedProse((const unsigned char*)all.data() + start, len);
            total++;
            if (p) flowed++;
            (void)artKept;
            start = end + 1;
        }
        double pct = 100.0 * flowed / total;
        printf("%-46s %6u lines, %5u flow (%2.0f%%)\n", argv[1], total, flowed, pct);
        // a guide that flows almost nothing means the rule is too strict; one
        // that flows almost everything means it is not protecting the art
        assert(pct > 5.0 && "almost nothing flowed - the rule is too strict");
        assert(pct < 90.0 && "nearly everything flowed - art is not being protected");
    }

    printf("guide text rules: all checks passed\n");
    return 0;
}
