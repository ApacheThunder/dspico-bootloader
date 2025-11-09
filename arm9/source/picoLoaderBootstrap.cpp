#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <nds/arm9/cache.h>
#include <libtwl/gfx/gfx.h>
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/mem/memVram.h>
#include <libtwl/dma/dmaNitro.h>
#include <libtwl/dma/dmaTwl.h>
#include <libtwl/ipc/ipcFifoSystem.h>
#include <libtwl/mem/memExtern.h>
#include "core/Environment.h"
#include "ipcChannels.h"
#include "errorDisplay/ErrorDisplay.h"
#include "fat/dldi.h"
#include "fat/ff.h"
#include "picoLoaderBootstrap.h"

#define PICO_LOADER_9_PATH    "/_pico/picoLoader9.bin"
#define PICO_LOADER_7_PATH    "/_pico/picoLoader7.bin"

#define PICOLOADER_BUFFER_SIZE (128 * 1024) // 128KB

typedef void (*pico_loader_9_func_t)(void);

static pload_params_t sLoadParams;
static PicoLoaderBootDrive sBootDrive;

static FATFS sFatFs;
static FIL sFile;

static uint8_t picoLoader9[PICOLOADER_BUFFER_SIZE];
static uint8_t picoLoader7[PICOLOADER_BUFFER_SIZE];

static void showErrorScreen(const char* text)
{
    // Enable DS display only on errors
    REG_DISPCNT = 0x10000;
    REG_DISPCNT_SUB = 0x10000;
    ErrorDisplay().PrintError(text);
    while (true);
}

static u32 loadFile(const char* path, void* destination)
{
    if (f_open(&sFile, path, FA_OPEN_EXISTING | FA_READ) != FR_OK)
    {
        showErrorScreen("ERROR: Failed to open Pico Loader file.");
    }

    u32 size = f_size(&sFile);

    UINT bytesRead;
    if (f_read(&sFile, destination, size, &bytesRead) != FR_OK)
    {
        showErrorScreen("ERROR: Failed to read Pico Loader file.");
    }

    f_close(&sFile);

    return size;
}

pload_params_t* pload_getLoadParams()
{
    return &sLoadParams;
}

void pload_setBootDrive(PicoLoaderBootDrive bootDrive)
{
    sBootDrive = bootDrive;
}

void pload_start()
{
    rtos_disableIrqs();
    REG_IME = 0;

    REG_DISPCNT = 0;
    REG_DISPCNT_SUB = 0;

    dma_ntrStopSafe(0);
    dma_ntrStopSafe(1);
    dma_ntrStopSafe(2);
    dma_ntrStopSafe(3);

    if (Environment::IsDsiMode())
    {
        REG_NDMA0CNT = 0;
        REG_NDMA1CNT = 0;
        REG_NDMA2CNT = 0;
        REG_NDMA3CNT = 0;
    }

    // Mount the SD card
    if (f_mount(&sFatFs, "fat:", 1) != FR_OK)
    {
        showErrorScreen("Error: Failed to mount SD card.");
    }

    // Load Pico Loader from the SD card
    mem_setVramAMapping(MEM_VRAM_AB_LCDC);
    mem_setVramCMapping(MEM_VRAM_C_LCDC);
    mem_setVramDMapping(MEM_VRAM_D_LCDC);

    u32 picoLoader9size = loadFile(PICO_LOADER_9_PATH, (void*)picoLoader9);
    u32 picoLoader7size = loadFile(PICO_LOADER_7_PATH, (void*)picoLoader7);

    DC_FlushAll();
    DC_InvalidateAll();
    IC_InvalidateAll();

    dma_ntrCopy16(3, picoLoader9, (void*)0x06800000, picoLoader9size);
    dma_ntrCopy16(3, picoLoader7, (void*)0x06840000, picoLoader7size);

    // Setup the Pico Loader header
    ((pload_header7_t*)0x06840000)->bootDrive = sBootDrive;
    dma_ntrCopy16(3, &sLoadParams, &((pload_header7_t*)0x06840000)->loadParams, sizeof(pload_params_t));
    ((pload_header7_t*)0x06840000)->dldiDriver = gDldiStub;

    // Boot into Pico Loader
    mem_setVramCMapping(MEM_VRAM_C_ARM7_00000);
    mem_setVramDMapping(MEM_VRAM_D_ARM7_20000);
    mem_setDsCartridgeCpu(EXMEMCNT_SLOT1_CPU_ARM7);
    ipc_sendFifoMessage(IPC_CHANNEL_LOADER, 1);
    ((pico_loader_9_func_t)0x06800000)();
}
