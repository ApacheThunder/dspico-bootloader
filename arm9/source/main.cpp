#include "common.h"
#include <libtwl/card/card.h>
#include <libtwl/mem/memExtern.h>
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/rtos/rtosThread.h>
#include <libtwl/ipc/ipcSync.h>
#include <libtwl/ipc/ipcFifoSystem.h>
#include "core/Environment.h"
#include "picoLoaderBootstrap.h"

#define NTR_CMD_ID_GAME_DISABLE_SCRAMBLING      0xFC00000000000000ull

/// @brief Switches the DSpico into unscrambled game mode and disables scrambling.
static void disableScrambling()
{
    // Map slot 1 to arm9
    mem_setDsCartridgeCpu(EXMEMCNT_SLOT1_CPU_ARM9);

    // Switch the DSpico into unscrambled game mode
    card_romSetCmd(NTR_CMD_ID_GAME_DISABLE_SCRAMBLING);
    card_romStartXfer(MCCNT1_DIR_READ | MCCNT1_RESET_OFF | MCCNT1_CLK_6_7_MHZ | MCCNT1_LEN_0 | MCCNT1_CMD_SCRAMBLE |
        MCCNT1_LATENCY2(0) | MCCNT1_CLOCK_SCRAMBLER | MCCNT1_LATENCY1(24), false);
    card_romWaitBusy();

    // Set the seed of the scrambler to zero. As a result, it will only ever produce zero's.
    // This means that even if a command is send with scrambling enabled, it will have no effect.
    REG_MCCNT1 = 0;
    REG_MCSCR0 = 0;
    REG_MCSCR1 = 0;
    REG_MCSCR2 = 0;
    REG_MCCNT1 = MCCNT1_RESET_OFF | MCCNT1_APPLY_SCRAMBLE_SEED | MCCNT1_CLOCK_SCRAMBLER | MCCNT1_READ_DATA_DESCRAMBLE;
}

int main(int argc, char* argv[])
{
    Environment::Initialize();
    mem_setDsCartridgeCpu(EXMEMCNT_SLOT1_CPU_ARM9);

    rtos_initIrq();
    rtos_startMainThread();
    ipc_initFifoSystem();

    while (ipc_getArm7SyncBits() != 7);

    disableScrambling();

    // Boot _picoboot.nds from the DSpico SD card.
    pload_setBootDrive(PLOAD_BOOT_DRIVE_DLDI);
    auto loadParams = pload_getLoadParams();
    strlcat(loadParams->romPath, "fat:/_picoboot.nds", sizeof(loadParams->romPath));
    loadParams->savePath[0] = 0;
    pload_start();

    while(1);
}
