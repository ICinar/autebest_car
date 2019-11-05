/**
 * \file      SHM_Serialized_Types.h
 * \brief     Types used to serialize CAN types.
 * \details   The CAN types mus be serialized when storing TX and RX data into
 *            shared memory. This file defines the types used for serialization.
 *
 * \date      24.11.2015
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

#if (!defined SHM_SERIALIZED_TYPES_H)
#define SHM_SERIALIZED_TYPES_H

/*==================[inclusions]==============================================*/

#include <ComStack_Types.h>

/*==================[macros]==================================================*/
/*==================[type definitions]========================================*/

/* CAN PDU serialized in shared memory.
 * We use generic types because the field sizes of a Can_PduType are
 * configurable, which might brake serialization. For instance it will break
 * if Can_IdType will have different sizes.
 */
typedef struct
{
    uint32 MessageBoxID;
    uint32 swPduHandle;
    uint32 length;
    uint32 id;
    uint8  sdu[8];
} Can_Serialized_PduType;

/*==================[external function declarations]==========================*/
/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/

#endif /* if (!defined SHM_SERIALIZED_TYPES_H) */
/*==================[end of file]=============================================*/
