/**
 * \file      vCan_GeneralTypes.h
 * \brief     Types and constants shared among the AUTOSAR CAN modules Can, CanIf and CanTrcv.
 * \details
 *
 * \see       http://www.autosar.org/fileadmin/files/releases/4-2/software-architecture/communication-stack/standard/AUTOSAR_SWS_CANDriver.pdf
 *
 * \date      19.11.2015
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

#if (!defined VCAN_GENERALTYPES_H)
#define VCAN_GENERALTYPES_H

/*==================[inclusions]==============================================*/

#include "SHM_Serialized_Types.h" /* Can_Serialized_PduType */

/*==================[macros]==================================================*/

#define CAN_EXTENDED_ADDRESSING     STD_ON
#define CAN_EXTENDED_HW_ADDRESSING  STD_ON

/*==================[type definitions]========================================*/

/* Do not define the Can types if they where already defined,
   mostly by including them from the AUTOSAR stack.
   Otherwise we get "error: conflicting types". */
#if (!defined CAN_TYPES_DEFINED)

#if(CAN_EXTENDED_ADDRESSING==STD_ON)
typedef uint32 Can_IdType;
#else
typedef uint16 Can_IdType;
#endif

#if(CAN_EXTENDED_HW_ADDRESSING==STD_ON)
typedef uint16 Can_HwHandleType;
#else
typedef uint8 Can_HwHandleType;
#endif


typedef enum
{
    CAN_T_START,
    CAN_T_STOP,
    CAN_T_SLEEP,
    CAN_T_WAKEUP
} Can_StateTransitionType;

typedef enum
{
    CAN_OK = 0u,
    CAN_NOT_OK,
    CAN_BUSY
} Can_ReturnType;

/* Configuration structure for Can_Write. */
typedef struct
{
    uint32                  ipev;
    Can_Serialized_PduType* pdu;
    PduIdType             * pduid;
} Can_Write_Config;

/* Configuration structure for CAN_RX_ISR. */
typedef struct
{
    Can_Serialized_PduType* pdu;
} Can_RX_ISR_Config;

/**
 * \brief Configuration of the virtual CAN driver.
 */
typedef struct
{
    uint32             vCan_Write_Entries_Count;
    Can_Write_Config * vCan_Write_Entries;

    uint32             vCan_RxISR_Entries_Count;
    Can_RX_ISR_Config* vCan_RxISR_Entries;

    PduIdType        * vCanTx_Confirmation_shm;
} Can_ConfigType;

typedef struct
{
    uint8 nothing;
} Can_ControllerBaudrateConfigType;

typedef struct
{
    PduIdType  swPduHandle;
    uint8      length;
    Can_IdType id;
    uint8*     sdu;
} Can_PduType;

#endif /* if (!defined CAN_TYPES_DEFINED) */

/*==================[external function declarations]==========================*/
/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/

#endif /* if (!defined VCAN_GENERALTYPES_H) */
/*==================[end of file]=============================================*/
