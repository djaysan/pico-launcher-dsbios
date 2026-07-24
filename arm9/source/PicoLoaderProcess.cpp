#include "common.h"
#include <nds/arm9/video.h>
#include "picoLoaderBootstrap.h"
#include "PicoLoaderProcess.h"

void PicoLoaderProcess::Run()
{
    REG_MASTER_BRIGHT = 0x401F;
    REG_MASTER_BRIGHT_SUB = 0x401F;
    REG_DISPCNT = 0;
    REG_DISPCNT_SUB = 0;
    // the strip's translucent pills keep the sub blend registers armed each
    // frame; hand the loader a clean slate like every other process exit
    REG_BLDCNT_SUB = 0;
    REG_BLDALPHA_SUB = 0;
    pload_start();
}
