#include "common.h"
#include <string.h>
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/ipc/ipcFifoSystem.h>
#include <libtwl/sound/sound.h>
#include "ipcChannels.h"
#include "picoLoader7.h"
#include "picoLoaderBootstrap.h"

typedef void (*pico_loader_7_func_t)(void);

static volatile bool sShouldStart = false;

static void ipcMessageHandler(u32 channel, u32 data, void* arg)
{
    if (data == 1)
    {
        sShouldStart = true;
    }
}

void pload_init()
{
    ipc_setChannelHandler(IPC_CHANNEL_LOADER, ipcMessageHandler, nullptr);
}

bool pload_shouldStart()
{
    return sShouldStart;
}

void pload_start()
{
    snd_setMasterEnable(false);
    rtos_disableIrqs();
    REG_IME = 0;
    auto header7 = (pload_header7_t*)0x06000000;
    ((pico_loader_7_func_t)header7->entryPoint)();
}