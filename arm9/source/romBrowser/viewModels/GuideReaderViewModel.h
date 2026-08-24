#pragma once
#include <memory>
#include "core/String.h"
#include "fat/File.h"
#include "gui/font/nitroFont2.h"
#include "../IRomBrowserController.h"

/// @brief Reads one guide, a screenful of wrapped lines at a time.
///
/// Guides run from ~100 KB to well over 1 MB and the DS has 4 MB total, with a
/// folder listing and cover art already in it, so the file is never loaded
/// whole. The model keeps a BYTE OFFSET for the first visible line and wraps
/// forward from it on demand. No line index, no full-file scan: opening the
/// 1.4 MB Chrono Trigger guide costs the same as opening the smallest one.
///
/// Scrolling back a line does not need a history either. Wrapping only depends
/// on where the SOURCE line starts, so the line above the view is found exactly
/// by rewinding to the previous newline and wrapping forward from there.
class GuideReaderViewModel
{
public:
    /// @brief Lines on screen. Capped by sub OBJ vram: bank I is 16 KB and one
    ///        224px line costs 1792 bytes of it, so nine is all that fits even
    ///        with the browser's own top screen view swapped out.
    /// ponytail: the sub BG (bank C, 128 KB, BG1_SUB/BG2_SUB free) lifts this,
    /// at the cost of its own blit path.
    static constexpr u32 kVisibleLines = 9;
    /// @brief Longest wrapped line. A full width of the narrowest glyphs still
    ///        fits well inside this.
    static constexpr u32 kMaxLineLength = 255;

    GuideReaderViewModel(IRomBrowserController* romBrowserController,
        const nft2_header_t* font, u32 lineWidth);

    /// @brief Opens a guide by file name inside the guides folder, saving the
    ///        position of whatever was open before. Resumes where this game was
    ///        last left off.
    void Open(const char* guideFileName);

    /// @brief Stores the reading position of the open guide, if any.
    void SavePosition();

    /// @brief False until a guide has been opened, or when one could not be
    ///        read (deleted, or unreadable).
    constexpr bool IsOpen() const { return _isOpen; }

    /// @brief Guide file name without its extension, for the header.
    const char* GetTitle() const { return _title.GetString(); }

    constexpr u32 GetLineCount() const { return _lineCount; }
    const char16_t* GetLine(u32 index) const { return _lines[index]; }

    /// @brief How far into the file the top of the view is, 0..100.
    u32 GetProgressPercent() const;

    /// @brief Bumped every time the view is re-laid-out, so a view can tell
    ///        whether its labels are stale without re-rendering every frame.
    constexpr u32 GetViewSerial() const { return _viewSerial; }

    /// @brief One line at a time - what the d-pad does.
    void ScrollDown();
    void ScrollUp();
    /// @brief A screenful at a time, less one line of overlap so nothing is
    ///        stepped over - what L and R do.
    void PageDown();
    void PageUp();

private:
    /// @brief Bytes read per layout. One screenful of the narrowest glyphs
    ///        cannot use more than 256 bytes a line, so this always holds the
    ///        whole view and the wrap never has to deal with a cut-off chunk.
    static constexpr u32 kChunkSize = 8192;
    /// @brief How far back to look for the newline that starts a source line.
    ///        A guide line is tens of bytes; this is generous. A "line" longer
    ///        than this wraps from an approximate start, which costs at most a
    ///        differently broken line while scrolling up through it.
    static constexpr u32 kBackScanSize = 4096;

    struct LineSpan
    {
        u32 start;
        u32 end;
    };

    IRomBrowserController* _romBrowserController;
    const nft2_header_t* _font;
    u32 _lineWidth;
    String<char, 128> _title;
    File _file;
    u32 _fileSize = 0;
    bool _isOpen = false;

    /// @brief Byte offset of the first line on screen.
    u32 _topOffset = 0;
    /// @brief Byte offset of the second line on screen, which is where the top
    ///        goes when scrolling down one line.
    u32 _secondLineOffset = 0;
    /// @brief Byte offset just past the last line on screen.
    u32 _nextViewOffset = 0;

    u32 _lineCount = 0;
    u32 _viewSerial = 0;
    std::unique_ptr<char16_t[]> _lineStorage;
    char16_t* _lines[kVisibleLines] = { };
    std::unique_ptr<u8[]> _chunk;

    u32 GlyphAdvance(u16 character) const;
    /// @brief Wraps up to \p maxLines lines starting at \p offset. Returns how
    ///        many it produced; \p nextOffset is where the line after the last
    ///        one starts.
    u32 WrapLines(u32 offset, u32 maxLines, LineSpan* spans, u32& nextOffset);
    void LayoutView();
    void BuildLine(u32 lineIndex, const u8* text, u32 start, u32 end);
    /// @brief Byte offset of the display line directly above \p offset.
    u32 PreviousLineStart(u32 offset);
};
