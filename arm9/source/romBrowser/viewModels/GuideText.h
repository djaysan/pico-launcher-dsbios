#pragma once

/// @brief Text rules for the guide reader, kept free of every DS header so
///        tools/guide-text-test.cpp can exercise them on the host against real
///        guide files.
namespace GuideText
{

/// @brief Decodes one UTF-8 code point at \p pos and advances it.
///        An invalid sequence decodes as its single byte, so a guide written in
///        Latin-1 still reads instead of turning to mush.
inline unsigned Decode(const unsigned char* text, unsigned size, unsigned& pos)
{
    unsigned c = text[pos];
    if (c < 0x80)
    {
        pos++;
        return c;
    }
    unsigned extra, cp;
    if ((c & 0xE0) == 0xC0) { extra = 1; cp = c & 0x1F; }
    else if ((c & 0xF0) == 0xE0) { extra = 2; cp = c & 0x0F; }
    else if ((c & 0xF8) == 0xF0) { extra = 3; cp = c & 0x07; }
    else { pos++; return c; }

    // a sequence running off the end of the buffer falls out of the loop below
    for (unsigned i = 1; i <= extra; i++)
    {
        if (pos + i >= size || (text[pos + i] & 0xC0) != 0x80)
        {
            pos++;              // not a continuation byte: emit the lead byte
            return c;
        }
        cp = (cp << 6) | (text[pos + i] & 0x3F);
    }
    pos += extra + 1;
    // the font is 16 bit; anything above the BMP becomes a replacement
    return cp > 0xFFFF ? 0xFFFD : cp;
}

/// @brief The shortest source line that can still be mid-paragraph. Guides are
///        hard wrapped around 70-80 columns, so a line much shorter than that
///        ended its paragraph on purpose - a heading, a list item, a last line.
constexpr unsigned kMinFlowedLength = 40;

/// @brief Does this source line look like hard wrapped prose that should flow
///        into the next one?
///
/// Guides are wrapped to a width no DS screen has, so honouring every newline
/// leaves a short orphan under every full line. Joining them fixes that, but
/// these files are also full of ascii tables, banners and maps, and reflowing
/// THOSE destroys them. So only lines that look like plain sentences join:
/// nothing indented, no column gaps, no box drawing.
/// @param knownLong Set when the caller can see only the TAIL of a line it
///        already knows is long, because the line began before the buffer. Skips
///        the length test rather than misjudging a wrapped line as a heading.
inline bool IsFlowedProse(const unsigned char* line, unsigned length, bool knownLong = false)
{
    // A one or two space margin is a writing style - plenty of guides indent
    // every prose line - so measure the text INSIDE it. Three or more is a
    // block quote, a list item or art, and must not flow.
    unsigned indent = 0;
    while (indent < length && line[indent] == ' ')
        indent++;
    if (indent >= 3 || indent >= length)
        return false;
    line += indent;
    length -= indent;

    if (!knownLong && length < kMinFlowedLength)
        return false;
    if (line[0] == '\t')
        return false;

    unsigned runOfSpaces = 0;
    unsigned artChars = 0;
    unsigned letters = 0;
    for (unsigned i = 0; i < length; i++)
    {
        unsigned char c = line[i];
        if (c == ' ')
        {
            runOfSpaces++;
            if (runOfSpaces >= 3)
                return false;       // column gap: a table or a layout
            continue;
        }
        runOfSpaces = 0;
        if (c == '\t')
            return false;
        switch (c)
        {
            case '|': case '+': case '=': case '_': case '~':
            case '*': case '#': case '<': case '>': case '\\':
            case '[': case ']': case '{': case '}': case '/':
                artChars++;
                break;
            default:
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
                    letters++;
                break;
        }
    }
    if (artChars > 2)
        return false;               // box drawing, a rule, or a banner
    return letters * 2 > length;    // mostly letters, so: a sentence
}

}
