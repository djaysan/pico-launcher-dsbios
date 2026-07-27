#pragma once
#include <cstring>
#include <memory>
#include "../../common/logger/IOutputStream.h"
#include "fat/File.h"

// Real hardware with no emulator/JTAG/nocash debug channel has nowhere else
// for LOG_* calls to go - without this they vanish into a NullLogger and a
// hardware-only crash leaves no trace at all. Opens, writes and closes the
// file on every call (instead of keeping a handle open) so that whatever was
// logged right up to a hard crash is actually on the SD card afterwards (the
// per-call close flushes via f_sync), not stuck in a buffer that never got
// flushed.
class FileOutputStream : public IOutputStream
{
    static constexpr const char* kLogPath = "/_pico/launcher.log";

    bool _truncatedThisSession = false;

public:
    void Write(const char* str) override
    {
        // File embeds a FIL with a 512-byte sector buffer, and f_open pushes
        // its own ~512-byte long-file-name working buffer - keep both off the
        // stack (this can run on a 2KB worker thread from a LOG_ERROR fired
        // deep in a call chain) by heap-allocating, exactly like the settings
        // and game-data serializers do for the same reason.
        auto file = std::make_unique<File>();
        BYTE mode = FA_WRITE | (_truncatedThisSession ? FA_OPEN_APPEND : FA_CREATE_ALWAYS);
        if (file->Open(kLogPath, mode) != FR_OK)
            return;
        _truncatedThisSession = true;

        u32 bytesWritten;
        file->Write(str, strlen(str), bytesWritten);
    }

    void Flush() override { }
};
