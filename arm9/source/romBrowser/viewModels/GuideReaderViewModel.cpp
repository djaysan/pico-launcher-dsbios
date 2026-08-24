#include "common.h"
#include <string.h>
#include <algorithm>
#include "core/StringUtil.h"
#include "GuidesViewModel.h"
#include "GuideReaderViewModel.h"

GuideReaderViewModel::GuideReaderViewModel(IRomBrowserController* romBrowserController,
    const nft2_header_t* font, u32 lineWidth)
    : _romBrowserController(romBrowserController)
    , _font(font)
    , _lineWidth(lineWidth)
{
    _lineStorage = std::make_unique_for_overwrite<char16_t[]>(kVisibleLines * (kMaxLineLength + 1));
    for (u32 i = 0; i < kVisibleLines; i++)
    {
        _lines[i] = &_lineStorage[i * (kMaxLineLength + 1)];
        _lines[i][0] = 0;
    }
    _chunk = std::make_unique_for_overwrite<u8[]>(kChunkSize);
}

void GuideReaderViewModel::Open(const char* guideFileName)
{
    // whatever was being read keeps its place before we move on
    SavePosition();
    if (_isOpen)
        _file.Close();
    _isOpen = false;
    _lineCount = 0;
    _topOffset = 0;
    _fileSize = 0;

    char title[129];
    StringUtil::Copy(title, guideFileName, sizeof(title));
    char* dot = strrchr(title, '.');
    if (dot)
        *dot = 0;
    _title = title;

    // the controller owns the path: it is also what resolves which game's entry
    // the reading position belongs to
    _romBrowserController->SetGuidePath(guideFileName);
    const char* guidePath = _romBrowserController->GetGuidePath();

    if (_file.Open(guidePath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
    {
        LOG_ERROR("Couldn't open guide %s\n", guidePath);
        _viewSerial++;
        return;
    }
    _fileSize = _file.GetSize();
    if (_fileSize == 0)
    {
        LOG_ERROR("Guide %s is empty\n", guidePath);
        _file.Close();
        _viewSerial++;
        return;
    }
    _isOpen = true;

    // Resume where this game was left off. The stored value is a line start
    // from a previous session, so the wrap picks up on exactly that line even
    // if the font, the wrap width or the number of visible lines has changed
    // since - which is why it is a byte offset and not a line number.
    u32 storedOffset = _romBrowserController->GetGuideReadOffset();
    _topOffset = storedOffset < _fileSize ? storedOffset : 0;
    LayoutView();
}

void GuideReaderViewModel::SavePosition()
{
    // Written when the guide is left or swapped, not on every scroll:
    // gamedata.json is rewritten whole on every save.
    if (_isOpen)
        _romBrowserController->SetGuideReadOffset(_topOffset);
}

// spacingLeft/spacingRight are signed and can pull glyphs together, so clamp:
// a zero or negative advance would let a line never reach its width.
u32 GuideReaderViewModel::GlyphAdvance(u16 character) const
{
    int glyphIdx = nft2_findGlyphIdxForCharacter(_font, character);
    const nft2_glyph_t* glyph = &_font->glyphInfoPtr[glyphIdx];
    int advance = glyph->spacingLeft + (int)glyph->glyphWidth + glyph->spacingRight;
    return advance > 0 ? (u32)advance : 1;
}

// Wraps from \p offset, in BYTES of the file throughout, so every offset it
// hands back stays valid whatever the font or the screen does.
u32 GuideReaderViewModel::WrapLines(u32 offset, u32 maxLines, LineSpan* spans, u32& nextOffset)
{
    nextOffset = offset;
    if (!_isOpen || maxLines == 0)
        return 0;

    if (_file.Seek(offset) != FR_OK)
    {
        LOG_ERROR("Couldn't seek guide to %u\n", offset);
        return 0;
    }
    u32 bytesRead = 0;
    if (_file.Read(_chunk.get(), kChunkSize, bytesRead) != FR_OK)
    {
        LOG_ERROR("Couldn't read guide at %u\n", offset);
        return 0;
    }

    const u8* text = _chunk.get();
    u32 pos = 0;
    u32 lineCount = 0;
    while (lineCount < maxLines && pos < bytesRead)
    {
        u32 lineStart = pos;
        u32 x = 0;
        int lastSpace = -1;
        u32 lineEnd = 0;
        bool broke = false;
        while (pos < bytesRead)
        {
            u8 c = text[pos];
            if (c == '\n')
            {
                lineEnd = pos;
                pos++;              // the newline belongs to this line
                broke = true;
                break;
            }
            if (c < 0x20 && c != '\t')
            {
                pos++;              // \r and friends take no width
                continue;
            }
            u32 advance = GlyphAdvance(c == '\t' ? ' ' : c);
            if (x + advance > _lineWidth)
            {
                if (lastSpace >= (int)lineStart)
                {
                    // break on the last space that fit, and swallow it
                    lineEnd = (u32)lastSpace;
                    pos = (u32)lastSpace + 1;
                }
                else
                {
                    // one unbroken run wider than the line: split it, and never
                    // stall - always take at least one byte
                    lineEnd = pos > lineStart ? pos : lineStart + 1;
                    pos = lineEnd;
                }
                broke = true;
                break;
            }
            x += advance;
            if (c == ' ')
                lastSpace = (int)pos;
            pos++;
        }
        if (!broke)
        {
            // end of file (or of the chunk, which kChunkSize makes impossible
            // for a single screenful)
            lineEnd = pos;
        }
        spans[lineCount].start = offset + lineStart;
        spans[lineCount].end = offset + lineEnd;
        lineCount++;
    }
    nextOffset = offset + pos;
    return lineCount;
}

void GuideReaderViewModel::LayoutView()
{
    _lineCount = 0;
    _secondLineOffset = _topOffset;
    _nextViewOffset = _topOffset;
    _viewSerial++;
    if (!_isOpen)
        return;

    // one extra span so the start of the line after the view is known too
    LineSpan spans[kVisibleLines + 1];
    u32 next = _topOffset;
    u32 count = WrapLines(_topOffset, kVisibleLines, spans, next);

    // re-read the chunk for the text itself: WrapLines left it in _chunk
    // starting at _topOffset, so the spans index straight into it
    for (u32 i = 0; i < count; i++)
    {
        BuildLine(i, _chunk.get(), spans[i].start - _topOffset, spans[i].end - _topOffset);
    }
    _lineCount = count;
    _nextViewOffset = next;

    // scrolling down one line puts the top on the second line. With only one
    // line in the view (a very short tail) the end of the view is the only
    // place left to go.
    _secondLineOffset = count >= 2 ? spans[1].start : next;
}

void GuideReaderViewModel::BuildLine(u32 lineIndex, const u8* text, u32 start, u32 end)
{
    char16_t* out = _lines[lineIndex];
    u32 length = 0;
    for (u32 i = start; i < end && length < kMaxLineLength; i++)
    {
        u8 c = text[i];
        if (c == '\t')
            c = ' ';
        else if (c < 0x20)
            continue;   // \r and any other control byte carries no glyph
        out[length++] = (char16_t)c;
    }
    out[length] = 0;
}

// The line above \p offset, found rather than remembered: wrapping only depends
// on where the SOURCE line starts, so rewinding to the previous newline and
// wrapping forward from there reproduces the same breaks exactly.
u32 GuideReaderViewModel::PreviousLineStart(u32 offset)
{
    if (!_isOpen || offset == 0)
        return 0;

    u32 scanEnd = offset - 1;               // last byte of the line above
    u32 scanStart = scanEnd > kBackScanSize ? scanEnd - kBackScanSize : 0;
    u32 scanLength = scanEnd - scanStart;
    u32 srcStart = scanStart;
    if (scanLength > 0)
    {
        if (_file.Seek(scanStart) != FR_OK)
            return 0;
        u32 bytesRead = 0;
        if (_file.Read(_chunk.get(), scanLength, bytesRead) != FR_OK)
            return 0;
        // strictly before scanEnd, so the newline that TERMINATES the line
        // above is not mistaken for the one that starts it
        for (u32 i = bytesRead; i > 0; i--)
        {
            if (_chunk[i - 1] == '\n')
            {
                srcStart = scanStart + i;
                break;
            }
        }
    }

    // wrap forward from the source line start and take the last display line
    // that begins before the view does
    u32 result = srcStart;
    while (true)
    {
        LineSpan spans[2];
        u32 next = srcStart;
        if (WrapLines(srcStart, 1, spans, next) == 0 || next <= srcStart)
            break;
        if (next >= offset)
        {
            result = srcStart;
            break;
        }
        result = next;
        srcStart = next;
    }
    return result;
}

u32 GuideReaderViewModel::GetProgressPercent() const
{
    if (_fileSize == 0)
        return 0;
    return (u32)(((u64)_topOffset * 100) / _fileSize);
}

void GuideReaderViewModel::ScrollDown()
{
    if (!_isOpen || _nextViewOffset >= _fileSize)
        return;     // the last line is already on screen
    _topOffset = _secondLineOffset;
    LayoutView();
}

void GuideReaderViewModel::ScrollUp()
{
    if (!_isOpen || _topOffset == 0)
        return;
    _topOffset = PreviousLineStart(_topOffset);
    LayoutView();
}

void GuideReaderViewModel::PageDown()
{
    if (!_isOpen || _nextViewOffset >= _fileSize)
        return;
    // one line of overlap, so nothing is stepped over between screenfuls
    _topOffset = _lineCount > 1 ? PreviousLineStart(_nextViewOffset) : _nextViewOffset;
    LayoutView();
}

void GuideReaderViewModel::PageUp()
{
    if (!_isOpen || _topOffset == 0)
        return;
    for (u32 i = 0; i + 1 < kVisibleLines && _topOffset > 0; i++)
        _topOffset = PreviousLineStart(_topOffset);
    LayoutView();
}
