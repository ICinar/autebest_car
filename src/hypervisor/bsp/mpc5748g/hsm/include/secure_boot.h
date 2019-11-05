/**
* \file secure_boot.h
*
* \brief The structures used for Secure Boot.
*
* \details
*
* \author easycore GmbH, 91058 Erlangen, Germany
*
* \version 
*
* \par  License
* Customer: @@LicenseCustomer@@,
* License type: @@LicenseType@@,
* Licensed for project: @@LicenseProject@@.

* Copyright 2016 easycore GmbH, All rights exclusively reserved for easycore GmbH,
* unless expressly agreed to otherwise.
*/

#ifndef SECURE_BOOT_H
#define SECURE_BOOT_H

/*==================[inclusions]==================================================================*/

/*==================[macros]======================================================================*/

/*==================[type definitions]============================================================*/
/* -----------------------------------------------------------------------------------------------*/
/**
 * \brief  Represents an entry in the Secure Boot table.
 *
 * Flags:
 * ------
 *
 * Bit 0: if set (=1), check before booting (on the HSM)
 * Bit 1: if set (=1), check while booting  (on the host)
 * Bit 2: if set (=1), check during runtinme (on the HSM)
 * 
 *                 _________________31-3__________________________2_____1______0___
 *                |                                           |HSM   |Host  |HSM   |
 *                |               unused                      |runtim|boot  |boot  |
 *                |___________________________________________|______|______|______|
 *
 *------------------------------------------------------------------------------------------------*/
typedef struct __attribute__((__packed__)) {
      unsigned int start;                /* start address of the memory region to check           */
      unsigned int size;                 /* size of memory region to check in bytes               */
      unsigned int  flags;               /* Bit 0: check during boot (if =1) (int b/c align.)     */ 
      unsigned char hash[32u];           /* the SHA2-256 hash of that area                        */
} SecureBootTableEntry;

typedef struct __attribute__((__packed__)) {
      unsigned char signature[256u];     /* the signed hash                                       */
      unsigned int  count_entries;       /* the actual number of entries                          */
      SecureBootTableEntry entries[8u];  /* the actual entries                                    */
} SecureBootTable;

/*==================[internal function declarations]==============================================*/

/*==================[external function declarations]==============================================*/
void secure_boot(void);

/*==================[external constants]==========================================================*/
extern const SecureBootTable secure_boot_table;

/*==================[internal constants]==========================================================*/

/*==================[external data]===============================================================*/

/*==================[internal data]===============================================================*/

/*==================[internal function definitions]===============================================*/

/*==================[external function definitions]===============================================*/

#endif /* !SECURE_BOOT_H */
/*==================[end of file]=================================================================*/       
