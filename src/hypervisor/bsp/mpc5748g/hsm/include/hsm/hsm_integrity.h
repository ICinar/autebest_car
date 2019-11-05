/**
* \file hsm_integrity.h
*
* \brief Header for the runtime integrity check.
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

#ifndef HSM_INTEGRITY_H
#define HSM_INTEGRITY_H

/*==================[inclusions]==================================================================*/
#include <secure_boot.h>

/*==================[macros]======================================================================*/

/*==================[type definitions]============================================================*/

/*==================[internal function declarations]==============================================*/

/*==================[external function declarations]==============================================*/
/* -----------------------------------------------------------------------------------------------*/
/**
 * \brief  Initialize the runtime integrity checker.
 *
 * \param chunk_size Size of a chunk to check with a single call in bytes.
 */
/* -----------------------------------------------------------------------------------------------*/
void init_runtime_check
(
   SecureBootTable *secure_boot_table,
   unsigned int chunk_size
);

/* -----------------------------------------------------------------------------------------------*/
/**
 * \brief  Implements one step of the runtime integrity checker. 
 *
 * The size of the step is given by the init function.
 */
/* -----------------------------------------------------------------------------------------------*/
void runtime_check
(
   void
);

/*==================[external constants]==========================================================*/

/*==================[internal constants]==========================================================*/

/*==================[external data]===============================================================*/

/*==================[internal data]===============================================================*/

/*==================[internal function definitions]===============================================*/

/*==================[external function definitions]===============================================*/

#endif /* !HSM_INTEGRITY_H */
/*==================[end of file]=================================================================*/       
