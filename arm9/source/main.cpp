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

#define	REG_KEYINPUT	(*(vuint16*)0x04000130)
#define KEYS_CUR ((~REG_KEYINPUT)&0x3ff)

typedef enum KEYPAD_BITS {
  KEY_A      = BIT(0),  //!< Keypad A button.
  KEY_B      = BIT(1),  //!< Keypad B button.
  KEY_SELECT = BIT(2),  //!< Keypad SELECT button.
  KEY_START  = BIT(3),  //!< Keypad START button.
  KEY_RIGHT  = BIT(4),  //!< Keypad RIGHT button.
  KEY_LEFT   = BIT(5),  //!< Keypad LEFT button.
  KEY_UP     = BIT(6),  //!< Keypad UP button.
  KEY_DOWN   = BIT(7),  //!< Keypad DOWN button.
  KEY_R      = BIT(8),  //!< Right shoulder button.
  KEY_L      = BIT(9),  //!< Left shoulder button.
} KEYPAD_BITS;

DTCM_DATA ALIGN(16) const char* PicoBootPath = "fat:/_picoboot.nds";
DTCM_DATA ALIGN(16) const char* R4BootPath = "fat:/_DS_MENU.DAT";
DTCM_DATA ALIGN(16) const char* MISC1BootPath = "fat:/MISC1.DAT";
DTCM_DATA ALIGN(16) const char* MISC2BootPath = "fat:/MISC2.DAT";
DTCM_DATA ALIGN(16) const char* HomebrewPath = "fat:/boot.nds";

DTCM_DATA ALIGN(16) const char* CurrentBootPath;

DTCM_DATA ALIGN(16) volatile u32 CurrentKey = 0;

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
	
	CurrentKey = KEYS_CUR;
	
	switch (CurrentKey) {
		case KEY_A: CurrentBootPath = HomebrewPath; break;
		case KEY_B: CurrentBootPath = R4BootPath; break;
		case KEY_START: CurrentBootPath = MISC1BootPath; break;
		case KEY_SELECT: CurrentBootPath = MISC2BootPath; break;
		default: CurrentBootPath = PicoBootPath;
	}
		
	strlcat(loadParams->romPath, CurrentBootPath, sizeof(loadParams->romPath));
    loadParams->savePath[0] = 0;
    pload_start();

    while(1);
}

