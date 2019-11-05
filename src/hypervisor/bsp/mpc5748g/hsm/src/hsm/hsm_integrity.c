/**
* \file hsm_integrity.c
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
#include <hsm_integrity.h>
#include <hsm_board.h>
#include <secure_boot.h>
#include <sha2_256.h>
#include <string.h>

/*==================[macros]======================================================================*/

/*==================[type definitions]============================================================*/
typedef struct {
   unsigned int chunk_size;
   SecureBootTable *secure_boot_table;
   unsigned int current_entry_idx;
   unsigned int current_position; /* currently not used :( */
   uint8 hash[32];
} HsmRuntimeIntegrityStateType;

/*==================[internal function declarations]==============================================*/

/*==================[external function declarations]==============================================*/

/*==================[external constants]==========================================================*/

/*==================[internal constants]==========================================================*/

/*==================[external data]===============================================================*/

/*==================[internal data]===============================================================*/
static unsigned char hsm_integrity_is_initialized = 0u;
static HsmRuntimeIntegrityStateType hsm_integrity_state;

/*==================[internal function definitions]===============================================*/
static void reset
(
   void
)
{
   hsm_board_reset();
}

/*==================[external function definitions]===============================================*/
void init_runtime_check
(
   SecureBootTable *secure_boot_table_ptr,
   unsigned int chunk_size
)
{
   if (hsm_integrity_is_initialized == 0u) 
   {
      hsm_integrity_state.secure_boot_table = secure_boot_table_ptr;
      hsm_integrity_state.chunk_size = chunk_size;
      hsm_integrity_state.current_entry_idx = 0;
      hsm_integrity_state.current_position = 0;

      hsm_integrity_is_initialized = 1u;
   }
}

void runtime_check
(
   void
)
{
   uint32 hash_size = 32u;
   SecureBootTableEntry *cur_entry;

   if (hsm_integrity_is_initialized)
   {
      cur_entry = &(hsm_integrity_state.secure_boot_table->entries[hsm_integrity_state.current_entry_idx]);
      if (cur_entry->flags & 0x4)
      {
         if (EC_Crypt_SHA2_256_Hash((const uint8 *) cur_entry->start,
                                     cur_entry->size,
                                     &(hsm_integrity_state.hash[0]),
                                     &hash_size) != EC_Crypt_E_OK)
         {
            /* something went horribly wrong */
            reset();
         }
         if (memcmp(&(hsm_integrity_state.hash[0]), &(cur_entry->hash[0]), 32u) != 0)
         {
            reset();
         }
      }

      hsm_integrity_state.current_entry_idx = (hsm_integrity_state.current_entry_idx + 1) % 
                                               hsm_integrity_state.secure_boot_table->count_entries;
   }
}

/*==================[end of file]=================================================================*/       
