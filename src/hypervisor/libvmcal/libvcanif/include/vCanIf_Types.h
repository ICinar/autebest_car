/**
 * \file      vCanIf_Types.h
 * \brief     Generic type definitions of CanIf.
 * \details
 *
 * \see       http://www.autosar.org/fileadmin/files/releases/4-2/software-architecture/communication-stack/standard/AUTOSAR_SWS_CANInterface.pdf
 *
 * \date      18.11.2015
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

#if (!defined VCANIF_TYPES_H)
#define VCANIF_TYPES_H

/*==================[inclusions]==============================================*/

#include "SHM_Serialized_Types.h" /* Can_Serialized_PduType */
#include "ComStack_Types.h"
#include "vCan_GeneralTypes.h"

/*==================[macros]==================================================*/
/*==================[type definitions]========================================*/

typedef enum
{
    CANIF_CS_UNINIT = 0U,
    CANIF_CS_SLEEP,
    CANIF_CS_STARTED,
    CANIF_CS_STOPPED
} CanIf_ControllerModeType;

typedef struct
{
    Can_Serialized_PduType* pdu;
} CanIf_Transmit_Config;

typedef struct
{
    uint32                  ipev;
    Can_Serialized_PduType* pdu;
} CanIf_RxIndication_Config;

typedef struct
{
    uint32     ipev;
    PduIdType* pduid;
} CanIf_TxConfirmation_Config;

typedef struct
{
    uint32                        vCanIf_Transmit_Entries_Count;
    CanIf_Transmit_Config       * vCanIf_Transmit_Entries;

    uint32                        vCanIf_RxIndication_Entries_Count;
    CanIf_RxIndication_Config   * vCanIf_RxIndication_Entries;
    
    uint32                        vCanIf_TxConfirmation_Entries_Count;
    CanIf_TxConfirmation_Config * vCanIf_TxConfirmation_Entries;
} CanIf_ConfigType;

/*==================[external function declarations]==========================*/
/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/

#endif /* if (!defined VCANIF_TYPES_H) */
/*==================[end of file]=============================================*/
