/**
 * \file      smpu.h
 * \brief     Interface declaration of the SMPU driver for MPC5748G.
 * \details   This files declares the interface functions and macros of the SMPU
 *            driver for MPC5748G (Calypso).
 *            It also defines macros to compute the address of the SMPU
 *            registers and certain special values of them. The addresses and
 *            the corresponding fields are specific to the MPC5748G.
 *
 * \par       SMPU Overview
 *            The SMPU is a double peripheral hardware unit on the Calypso board
 *            which supervises the bus traffic and triggers an exception when it
 *            detects an access error due to non maching access rights. To
 *            specify the access writes for each bus master, the user of the
 *            SMPU uses 16 region descriptors. This module implements writing,
 *            reading the managing the 16 SMPU region descriptors. Is does not
 *            check any access rights.
 *
 *            This module works together with the module mpu on kernel level.
 *            In that module the function arch_mpu_part_switch overrides the
 *            contents of the SMPU unit except the regions with fix values.
 *
 * \par       Content of the SMPU regions in singlecore mode:
 *            \code
 *            +--------------------------------------------------------+
 *            | NvM and peripherals protection (SMPU0)                 |
 *            +--------------------------------------------------------+
 *            | region | usage                                         |
 *            +--------------------------------------------------------+
 *            | 0      |supervisor has all rights over flash           |
 *            | 1      |supervisor has all rights over peripherals     |
 *            | 2 - 15 |generated rights for user partitions           |
 *            +--------------------------------------------------------+
 *            +--------------------------------------------------------+
 *            | RAM protection (SMPU1)                                 |
 *            +--------------------------------------------------------+
 *            | 0      |supervisor has all rights over RAM             |
 *            | 1 - 15 |generated rights for user partitions           |
 *            +--------------------------------------------------------+
 *            \endcode
 *
 * 
 * \par       Content of the SMPU regions in multicore mode
 *            \code
 *            +--------------------------------------------------------+
 *            | RAM protection (SMPU1)                                 |
 *            +--------------------------------------------------------+
 *            | region  | usage                                        |
 *            +--------------------------------------------------------+
 *            | 0       |kernel global data, read only                 |
 *            | 1       |kernel data on CPU0, write for supervisor     |
 *            | 2 - 5   |partition data on CPU0, generated rights      |
 *            | 6       |kernel data on CPU1, write for supervisor     |
 *            | 7 - 10  |partition data on CPU1, generated rights      |
 *            | 11      |kernel data on CPU1, write for supervisor     |
 *            | 12 - 15 |partition data on CPU1, generated rights      |
 *            +--------------------------------------------------------+
 *            +--------------------------------------------------------+
 *            | NvM and peripherals protection (SMPU0)                 |
 *            +--------------------------------------------------------+
 *            | 0       |supervisor all writes over flash              |
 *            | 1       |supervisor all writes peripheral space        |
 *            | 2 - 5   |generated user partition rights on CPU0       |
 *            | 7 - 10  |generated user partition rights on CPU1       |
 *            | 12 - 15 |generated user partition rights on CPU2       |
 *            +--------------------------------------------------------+
 *            \endcode
 *
 * \par       Further documentation
 *            For more details regarding the SMPU unit see document
 *            "MPC5748G Reference Manual", "21.3 Memory map/register definition".
 *            The register definitions and some of the comments are taken from
 *            that document.
 *            For more details regarding the SMPU unit see document
 *            "MPC5748G Reference Manual",
 *            "Chapter 21, System Memory Protection Unit (SMPU)".
 *
 * \see       ppc_smpu.h, mpu.c, Calypso_Multicore_SMPU.png
 *
 * \date      20.08.2015
 * \author    Liviu Beraru <Liviu.Beraru@easycore.com>
 * \author    easycore GmbH, 91058 Erlangen, Germany
 * \version
 * \par       License
 * Customer:     @@LicenseCustomer@@,
 * License type: @@LicenseType@@,
 * Licensed for project: @@LicenseProject@@.
 *
 * 
 * \copyright Copyright 2015 easycore GmbH, 91058 Erlangen, Germany.
 * All rights exclusively reserved for easycore GmbH, unless expressly agreed
 * to otherwise.
 */

#if (!defined SMPU_H)
#define SMPU_H

/*==================[inclusions]==============================================*/
#include <stdint.h> /* uint32_t */
#include <arch_mpu_state.h> /* struct arch_mpu_region */
/*==================[macros]==================================================*/

/*==================[Base addresses]==========================================*/

/* Base adresses of SMPU0 and SMPU1.
 * You should use one of these base addresses for the argument x of the
 * following macros.
 */

/* SMPU0 protects flash and peripherals. */
#define SMPU0                (0xFC010000u)
/* SMPU1 protects RAM. */
#define SMPU1                (0xFC014000u)

/*==================[Memory regions]==========================================*/

/* See table Table "Table 3-1. System memory map"
 * in "MPC5748G Reference Manual", page 113. */
 
/* Start and end addresses of the system RAM. */
#define BOARD_RAM_BEGIN             (0x40000000u)
#define BOARD_RAM_END               (0x400BFFFFu)

/* Start and end address of all flash blocks. */
#define BOARD_NVM_BEGIN             (0x00F90000u)
#define BOARD_NVM_END               (0x0157FFFFu)

/* Start and end address of the Peripheral Address Space */
#define BOARD_PERIPHERAL_BEGIN      (0xF8000000u)
#define BOARD_PERIPHERAL_END        (0xFFFFFFFFu)

#if (defined SMP)
/* In multicore mode we split the RAM memory in 3 segments, one for each CPU.
 * Every CPU is allowed to read the entire memory but it can write only its own
 * memory segment.
 *
 *      Start      End
 * CPU0 0x40000000 0x4003FFFF
 * CPU1 0x40040000 0x4007FFFF
 * CPU2 0x40080000 0x400BFFFF
 */
#define RAM_BEGIN_CPU0 (0x40000000u)
#define RAM_END_CPU0   (0x40040000u)

#define RAM_BEGIN_CPU1 (0x40040000u)
#define RAM_END_CPU1   (0x40080000u)

#define RAM_BEGIN_CPU2 (0x40080000u)
#define RAM_END_CPU2   (0x400C0000u)
#endif


#define IS_RAM_ADDRESS(x) (((x) >= BOARD_RAM_BEGIN) && ((x) <= BOARD_RAM_END))

/*==================[Registers of the regions descriptors]====================*/

/*------------------[CES0]----------------------------------------------------*/

/* Control/Error Status Register 0.
 * CESR0 provides error status plus configuration information.
 * A global SMPU enable/disable bit is also included in this register.
 */

#define SMPU_0_CES0            (*(volatile unsigned int*) (0xFC010000u))
#define SMPU_1_CES0            (*(volatile unsigned int*) (0xFC014000u))

#define SMPU_CES0_ENABLE       (0x01u)
#define SMPU_CES0_DISABLE      (0x00u)

/*------------------[CES1]----------------------------------------------------*/

/* Control/Error Status Register 1.
 * CESR1 provides additional error status plus configuration information.
 */

#define SMPU_0_CES1            (*(volatile unsigned int*) (0xFC010004u))
#define SMPU_1_CES1            (*(volatile unsigned int*) (0xFC014004u))

/*------------------[Word 0]--------------------------------------------------*/
/**
 * \brief SMPU x, Region Descriptor n, Word 0.
 * \details The first word of the region descriptor defines the byte start
 * address of the memory region. Writes to this register clear the region
 * descriptor's valid bit (RGDn_WORD5[VLD]).
 */
#define SMPUx_RGDn_WRD0(x,n) (*(volatile unsigned int*) ((x) + 0x0400u + (64*(n))))

/*------------------[Word 1]--------------------------------------------------*/
/**
 * \brief SMPU x, Region Descriptor n, Word 1.
 * \details The second word of the region descriptor defines the end address of
 * the memory region. Writes to ANY word of the descriptor clears the valid bit
 * (RGDn_WORD5[VLD]).
 */
#define SMPUx_RGDn_WRD1(x,n) (*(volatile unsigned int*) ((x) + 0x0404u + (64*(n))))

/*------------------[Word 2]--------------------------------------------------*/
/**
 * \brief  SMPU x, Region Descriptor n, Word 2.
 * \details For word 2 of all regions we use the format 1 because it allows to
 * specify execution rights. RGD_WORD2_FMT1 defines pointers for each master to
 * select access control flags contained in Word3. For these fields, the bus
 * master number refers to the logical bus master number. RGD_WORD2_FMT1 applies
 * when RGD_WORD3[FMT] = 1.
 */
#define SMPUx_RGDn_WRD2(x,n) (*(volatile unsigned int*) ((x) + 0x0408u + (64*(n))))

/* Default value of word 2: point all bus masters to ACCSET1 in word 3.
 * That is, all 16 MxS fields (2 bit each) get the value 1. */
#define SMPU_ALL_POINT_ACCSET1 (0x55555555u)

#if (defined SMP)
/* Values of word 3 (access pointer) when each CPU points to ACCSET1 and all
 * others point to nothing (no rights) */

#define SMPU_CPU0_POINTS_ACCSET1    (0x40000000u)
#define SMPU_CPU1_POINTS_ACCSET1    (0x10000000u)
#define SMPU_CPU2_POINTS_ACCSET1    (0x04000000u)

#endif

/*------------------[Word 3]--------------------------------------------------*/
/**
 * \brief  SMPU x, Region Descriptor n, Word 3.
 * \details The third word of the SMPU region descriptor contains the format
 * select (FMT), three sets of access permissions flags used when FMT = 1, and
 * the cache inhibit bit. For access permissions used when FMT = 1, there are
 * three flags {read, write, execute} for each operating mode (supervisor,
 * user). 
 */
#define SMPUx_RGDn_WRD3(x,n) (*(volatile unsigned int*) ((x) + 0x040Cu + (64*(n))))

/* Make values for the fields ACCSET1, ACCSET2 and ACCSET3
 * (Access controls, Set 1 through Set 3). */
#define SMPU_WRD3_MAKE_ACCSET1(x) (((x) & 0x3F) << 26)
#define SMPU_WRD3_MAKE_ACCSET2(x) (((x) & 0x3F) << 20)
#define SMPU_WRD3_MAKE_ACCSET3(x) (((x) & 0x3F) << 14)

/* Bit masks for the different access rights of a ACCSETx field.
 * Usage: combine the values as needed and give the result to one of the macros
 * SMPU_WRD3_MAKE_ACCSETx above.
 */

#define ACCSET_S_READ             (0x20u) /* Supervisor read */
#define ACCSET_S_WRITE            (0x10u) /* Supervisor write */
#define ACCSET_S_EXEC             (0x08u) /* Supervisor execute */
#define ACCSET_U_READ             (0x04u) /* User read */
#define ACCSET_U_WRITE            (0x02u) /* User write */
#define ACCSET_U_EXEC             (0x01u) /* User execute */

/* Possible values of the FMT bit.
 * This field determines the format of the access control rights
 * specified in Word 2. */

/* Access rights are defined directly in Word 2. */
#define SMPU_WRD3_FMT0            (0x00u)
/* Access rights are defined in Word 3 in the ACCSET fiels. */
#define SMPU_WRD3_FMT1            (0x10u)

/*------------------[Word 5]--------------------------------------------------*/
/**
 * \brief SMPU x, Region Descriptor n, Word 5.
 * \details The sixth word of the MPU region descriptor contains the master ID
 * of the master who has locked the RGD as well as the Lock function and Valid
 * bit. If LCK is set, the master ID of the master who has locked it will be
 * stored here. Writes to ANY word of the descriptor clears the valid bit.
 */
#define SMPUx_RGDn_WRD5(x,n) (*(volatile unsigned int*) ((x) + 0x0414u + (64*(n))))

/*------------------[Other constants]-----------------------------------------*/
/**
 * \brief Number of region descriptors.
 */
#define REGIONS (16u)


/*==================[type definitions]========================================*/
/*==================[external function declarations]==========================*/

/*------------------[Invalidate an MPU region]--------------------------------*/

/**
 * \brief   Invalidate region descriptor.
 * \details This function invalidates the specified region despriptor by setting
 *          the corresponding bit SMPUx_RGDn_WRD5[VLD] to zero. It also sets all
 *          other words of the region descriptor to zero.
 *
 * \pre     The SMPU unit specified by the argument smpu must be disabled when
 *          calling this function (SMPUx_CES0[GVLD] must be zero).
 * \post    The region descriptor is not valid (is deactivated).
 *          Meaning, its data is not used by the SMPU when evaluating a bus
 *          access (is not input for the 'access evaluation macro').
 *
 * \param[in] smpu   Base address of the SMPU unit to use.
 *                   Must be SMPU0 or SMPU1.
 * \param[in] region Region number.
 */
void smpu_invalidate_region(uint32_t smpu, uint32_t region);

/*------------------[Write an MPU region]-------------------------------------*/

/**
 * \brief   Write region descriptor to the specified SMPU unit.
 * \pre     The SMPU unit specified by the argument smpu must be disabled when
 *          calling this function (SMPUx_CES0[GVLD] must be zero).
 * \param[in] smpu   Base address of the SMPU unit to use.
 *                   Must be SMPU0 or SMPU1.
 * \param[in] region Region number.
 * \param[in] region_desc Region descriptor structure containing the start
 *                        address, end address and permissions for the specified
 *                        region descriptor. The permissions must be in range
 *                        0x00 to 0x34 (6 bit permission field) as this is the
 *                        value which is written to SMPUx_RGDn_WRD3[ACCSET1].
 */
void smpu_write_region
(
  uint32_t smpu,
  uint32_t region,
  const struct arch_mpu_region * region_desc
);


/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/




#endif /* if (!defined SMPU_H) */
/*==================[end of file]=============================================*/

