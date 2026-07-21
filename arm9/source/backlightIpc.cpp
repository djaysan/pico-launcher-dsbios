#include "common.h"
#include <libtwl/ipc/ipcFifoSystem.h>
#include "ipcChannels.h"
#include "backlightIpc.h"

void backlight_setLevel(unsigned int level)
{
    ipc_sendFifoMessage(IPC_CHANNEL_PMIC, level & 3);
}
