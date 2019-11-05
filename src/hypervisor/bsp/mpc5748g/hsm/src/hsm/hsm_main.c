/**
* \file hsm_main.c
*
* \brief The HSM main function.
*
* \author easycore GmbH, 91058 Erlangen, Germany
*
* \par  License
* Customer: @@LicenseCustomer@@,
* License type: @@LicenseType@@,
* Licensed for project: @@LicenseProject@@.

* Copyright 2015 easycore GmbH. All rights exclusively reserved for easycore GmbH,
* unless expressly agreed to otherwise.
*/
/*==================[inclusions]==================================================================*/
#include <Hsm.h>
#include <hsm2ht.h>
#include <hsm_integrity.h>

/*==================[macros]======================================================================*/

/*==================[type definitions]============================================================*/

/*==================[internal function declarations]==============================================*/

/*==================[external function declarations]==============================================*/

/*==================[external constants]==========================================================*/

/*==================[internal constants]==========================================================*/

/*==================[external data]===============================================================*/

/*==================[internal data]===============================================================*/

/*==================[internal function definitions]===============================================*/

/*==================[external function definitions]===============================================*/
void hsm_main(void)
{
	unsigned int t;
	EC_Crypt_HsmCmdType *c;

	while (42)
	{
		HSM2HTS = HSMSTATE_IDLE;
		HSM2HTF = 1;
		t = HT2HSMF;
      /* waits for new instructions */
		while (t != 4)
      {
         t = HT2HSMF;
#ifdef SECURE_BOOT
         /* execute one step of the runtime integrity check */
         runtime_check();
#endif
      }
		HT2HSMF = t; /* clear flag */
		HSM2HTS = HSMSTATE_RUNNING;
		HSM2HTF = 1;
		/* HT2HSMS contains a pointer to the command and data */
		c = (EC_Crypt_HsmCmdType *) HT2HSMS;
		c->result = EC_Crypt_HsmExecuteCmd(c->cmd, c->data);
		/* flush write buffer to make data visible on the system bus */
		HSBI_BCR = 1;
		HSBI_BCR = 3;
	}
}

/*==================[end of file]=================================================================*/       
