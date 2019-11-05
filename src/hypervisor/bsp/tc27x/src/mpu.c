/*
 * mpu.c
 *
 * Tricore MPU handling
 *
 * azuepke, 2014-10-25: initial
 * azuepke, 2015-02-23: implementation
 * azuepke, 2015-03-06: use fix values for the kernel
 */

#include <board_stuff.h>
#include <tc_regs.h>

/** initialize the MPU */
__init void board_mpu_init(void)
{
	unsigned int syscon;

	/* load five task windows for data */
	MTCR(CSFR_DPRL(11), BOARD_FLASH_BASE);
	MTCR(CSFR_DPRU(11), BOARD_FLASH_BASE + BOARD_FLASH_SIZE);
	MTCR(CSFR_DPRL(12), BOARD_IO_BASE);
	MTCR(CSFR_DPRU(12), BOARD_IO_BASE + BOARD_IO_SIZE);
	MTCR(CSFR_DPRL(13), BOARD_CPU0_DRAM_BASE);
	MTCR(CSFR_DPRU(13), BOARD_CPU0_DRAM_BASE + BOARD_CPU0_DRAM_SIZE);
	MTCR(CSFR_DPRL(14), BOARD_CPU1_DRAM_BASE);
	MTCR(CSFR_DPRU(14), BOARD_CPU1_DRAM_BASE + BOARD_CPU1_DRAM_SIZE);
	MTCR(CSFR_DPRL(15), BOARD_CPU2_DRAM_BASE);
	MTCR(CSFR_DPRU(15), BOARD_CPU2_DRAM_BASE + BOARD_CPU2_DRAM_SIZE);

	/* load three task windows for code */
	MTCR(CSFR_CPRL(5), BOARD_FLASH_BASE);
	MTCR(CSFR_CPRU(5), BOARD_FLASH_BASE + BOARD_FLASH_SIZE);
	MTCR(CSFR_CPRL(6), BOARD_FLASH_UC_BASE);
	MTCR(CSFR_CPRU(6), BOARD_FLASH_UC_BASE + BOARD_FLASH_UC_SIZE);
	MTCR(CSFR_CPRL(7), BOARD_LOCAL_CRAM_BASE);
	MTCR(CSFR_CPRU(7), BOARD_LOCAL_CRAM_BASE + BOARD_LOCAL_CRAM_SIZE);

	/* update read, write and exec permissions */
	MTCR(CSFR_DPRE(0), 0xffff);
	MTCR(CSFR_DPRE(1), 0x0fff);
	MTCR(CSFR_DPRE(2), 0);
	MTCR(CSFR_DPRE(3), 0);

	MTCR(CSFR_DPWE(0), 0xf000);	/* flash (range 11) is not writeable */
	MTCR(CSFR_DPWE(1), 0);
	MTCR(CSFR_DPWE(2), 0);
	MTCR(CSFR_DPWE(3), 0);

	MTCR(CSFR_CPXE(0), 0x00e0);
	MTCR(CSFR_CPXE(1), 0x001f);
	MTCR(CSFR_CPXE(2), 0);
	MTCR(CSFR_CPXE(3), 0);

	/* turn on MPU */
	syscon = MFCR(CSFR_SYSCON);
	syscon |= SYSCON_PROTEN;
	MTCR_ISYNC(CSFR_SYSCON, syscon);
}
