#include "common.h"
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/rtos/rtosThread.h>
#include <libtwl/rtos/rtosEvent.h>
#include <libtwl/ipc/ipcSync.h>
#include <libtwl/ipc/ipcFifoSystem.h>
#include <libtwl/gfx/gfxStatus.h>
#include "picoLoaderBootstrap.h"

static rtos_event_t sVBlankEvent;

static void vblankIrq(u32 irqMask)
{
    rtos_signalEvent(&sVBlankEvent);
}

int main()
{
    rtos_initIrq();
    rtos_startMainThread();
    ipc_initFifoSystem();

    pload_init();

    rtos_createEvent(&sVBlankEvent);
    rtos_setIrqFunc(RTOS_IRQ_VBLANK, vblankIrq);
    rtos_enableIrqMask(RTOS_IRQ_VBLANK);

    gfx_setVBlankIrqEnabled(true);

    ipc_setArm7SyncBits(7);

    while (true)
    {
        rtos_waitEvent(&sVBlankEvent, true, true);

        if (pload_shouldStart())
        {
            pload_start();
        }
    }

    return 0;
}
