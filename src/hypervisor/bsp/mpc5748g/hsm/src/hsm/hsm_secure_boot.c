/**
* \file hsm_secure_boot.c
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

* Copyright 2016 easycore GmbH, All rights exclusively reserved for easycore GmbH,
* unless expressly agreed to otherwise.
*/
/*==================[inclusions]==================================================================*/
#include <secure_boot.h>
#include <hsm_secure_boot.h>
#include <string.h>             /* memcpy */
#include <sha2_256.h>
#include <hsm_integrity.h>
#include <rsa-pkcs.h>
#include <root_public_key.h>

/*==================[macros]======================================================================*/
#define HOST_SECURE_BOOT_TABLE_PTR ((unsigned int *) 0xFC0020)

/*==================[type definitions]============================================================*/

/*==================[internal function declarations]==============================================*/

/*==================[external function declarations]==============================================*/

/*==================[external constants]==========================================================*/

/*==================[internal constants]==========================================================*/

/*==================[external data]===============================================================*/
SecureBootTable hsm_internal_secure_boot_table;

/*==================[internal data]===============================================================*/



/*==================[internal function definitions]===============================================*/
/* -----------------------------------------------------------------------------------------------*/
/**
 * \brief  Secure Boot process failed, so we loop forever.
 */
/* -----------------------------------------------------------------------------------------------*/
static void loop_forever
(
   void
)
{
   while(42) { /* loop forever */ }
}

/* -----------------------------------------------------------------------------------------------*/
/**
 * \brief  Checks the signature
 *
 * If an error occurs or the signature is not valid this function never returns.
 */
/* -----------------------------------------------------------------------------------------------*/
static void check_signature
(
   void
)
{
   boolean result = 0;

   /* FIXME  change the struct, this check can fail if the order of the struct entries differs! */
   if (EC_Crypt_RsassaPkcs1v15_Verify(&hsm_internal_secure_boot_table.signature[0],
                                      sizeof hsm_internal_secure_boot_table.signature,
                                      (const uint8 *) &hsm_internal_secure_boot_table.count_entries,
                                      sizeof hsm_internal_secure_boot_table.count_entries +
                                      sizeof hsm_internal_secure_boot_table.entries,
                                      &root_public_key.modulus[0],
                                      sizeof root_public_key.modulus,
                                      &root_public_key.exponent[0],
                                      sizeof root_public_key.exponent,
                                      &result) != EC_Crypt_E_OK)
   {
      /* verification process failed */
      loop_forever();
   }
   if (result == 0)
   {
      loop_forever();
   }

}

/*==================[external function definitions]===============================================*/
void hsm_secure_boot
(
   void
)
{
   unsigned int i = 0u;
   uint8 hash[32] = { 0u };
   uint32 hash_len = 32u;
   SecureBootTableEntry *e;
   const void *host_secure_boot_table_addr = (const void *) *(HOST_SECURE_BOOT_TABLE_PTR);

   /* copy the table into our secured area */
   memcpy(&hsm_internal_secure_boot_table,
          host_secure_boot_table_addr,
          sizeof(hsm_internal_secure_boot_table));

   /* signature check does not return if failed */
   check_signature(); 
   /* valid signature, we are safe */

   /* init runtime integration check */
   init_runtime_check(&hsm_internal_secure_boot_table, 128u);

   for (i = 0u; i < hsm_internal_secure_boot_table.count_entries; i++)
   {
      e = &(hsm_internal_secure_boot_table.entries[i]);
      /* check if entry has to be checked during Secure Boot */
      if (e->flags & 0x1u)
      {
         /* Calculate the hash for the current memory region */
         if (EC_Crypt_SHA2_256_Hash((const uint8 *) e->start,
                                    e->size,
                                    &(hash[0u]), 
                                    &hash_len) != EC_Crypt_E_OK)
         {
            /* something went horribly wrong */
            loop_forever();
         }

         /* Compare the hash with the stored one */
         if (memcmp(&(hash[0u]), &(e->hash[0u]), 32u) != 0u)
         {
            loop_forever();
         }
      }
   }
}

/*===================[end of file]================================================================*/       
