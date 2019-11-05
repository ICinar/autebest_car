/**
* \file Hsm.h
*
* \brief Interface to the HSM.
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

* Copyright 2015 easycore GmbH
* All rights exclusively reserved for easycore GmbH, unless expressly agreed to otherwise.
*/

#ifndef HSM_H
#define HSM_H

/*==================[inclusions]==================================================================*/

/*==================[macros]======================================================================*/
#define HSM_VERSION  0x42

/* HSM states */
#define HSMSTATE_INIT      0xBAFF
#define HSMSTATE_IDLE      0xAFFE
#define HSMSTATE_RUNNING   0xBADE

/*==================[type definitions]============================================================*/
typedef enum
{
   AES_NATIVE_ECB_ENCRYPT_START  = 0x01,
   AES_NATIVE_ECB_ENCRYPT_UPDATE = 0x02,
   AES_NATIVE_ECB_ENCRYPT_FINISH = 0x03,
   AES_NATIVE_ECB_DECRYPT_START  = 0x11,
   AES_NATIVE_ECB_DECRYPT_UPDATE = 0x12,
   AES_NATIVE_ECB_DECRYPT_FINISH = 0x13,
   SECURE_FLASH                  = 0xa0,
   SECURE_DEBUG                  = 0xb0,
} EC_Crypt_HsmPrimitiveType;

typedef struct
{
   EC_Crypt_HsmPrimitiveType cmd;
   void *data;
   unsigned int result;
} EC_Crypt_HsmCmdType;

/*==================[external function declarations]==============================================*/
unsigned int EC_Crypt_HsmExecuteCmd
(
   EC_Crypt_HsmPrimitiveType cmd,
   void *vdp
);
/*==================[internal function declarations]==============================================*/

/*==================[external constants]==========================================================*/

/*==================[internal constants]==========================================================*/

/*==================[external data]===============================================================*/

/*==================[internal data]===============================================================*/

/*==================[internal function definitions]===============================================*/

/*==================[external function definitions]===============================================*/

#endif /* !HSM_H */
/*==================[end of file]=================================================================*/       
