/**
* \file Cry_Host2Hsm.c
*
* \brief
*
* \details
*
* \version
*
* \author easycore GmbH, 91058 Erlangen, Germany
*
* \par  License
* Customer: @@LicenseCustomer@@,
* License type: @@LicenseType@@,
* Licensed for project: @@LicenseProject@@.

* Copyright 2015 easycore GmbH, All rights exclusively reserved for easycore GmbH,
* unless expressly agreed to otherwise.
*/
/*==================[inclusions]==================================================================*/
#include <Cry_Host2Hsm.h>
#include <ht2hsm.h> 
#include <hv.h>
#include "app.id.h"

/*==================[macros]======================================================================*/
#define REG_WRITE32(address, value) (sys_kldd_call(CFG_KLDD_REG_WRITE32,address,value,0))
#define REG_READ32(address)         (sys_kldd_call(CFG_KLDD_REG_READ32,\
                                                   address,\
                                                   (unsigned int) &reg_read32_output,\
                                                   0), reg_read32_output)


/*==================[type definitions]============================================================*/

/*==================[internal function declarations]==============================================*/

/*==================[external function declarations]==============================================*/

/*==================[external constants]==========================================================*/
volatile uint32 reg_read32_output;

/*==================[internal constants]==========================================================*/

/*==================[external data]===============================================================*/

/*==================[internal data]===============================================================*/

/*==================[external function definitions]===============================================*/
void EC_Crypt_DoHsmCmd(EC_Crypt_HsmCmdType *cmd)
{
   /* TODO macros for the used Hardware */

	while (REG_READ32((uint32) HSM2HTF_A) == 0)              { /* wait */ }
	while (REG_READ32((uint32) HSM2HTS_A) != HSMSTATE_IDLE)  { /* wait */ }
   REG_WRITE32((uint32) HSM2HTF_A, 0xFFFFFFFFu);              /* clear flag */

	REG_WRITE32((uint32) HT2HSMS_A, (unsigned int) cmd);       /* commit cmd */
	REG_WRITE32((uint32) HT2HSMF_A, 4);                        /* inform the HSM */

	while (REG_READ32((uint32) HSM2HTF_A) == 0)              { /* wait */ }

	while (REG_READ32((uint32) HSM2HTS_A) != HSMSTATE_IDLE)  { /* wait */ }
}

/*==================[internal function definitions]===============================================*/

/*==================[end of file]=================================================================*/       
