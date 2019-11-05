/**
 * \file      CanIf.h
 * \brief     CAN Interface.
 * \details
 *
 * \see       http://www.autosar.org/fileadmin/files/releases/4-2/software-architecture/communication-stack/standard/AUTOSAR_SWS_CANInterface.pdf
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

#if (!defined CANIF_H)
#define CANIF_H

/*==================[inclusions]==============================================*/

/* Includes ComStack_Types.h, Std_Types.h */
#include "vCanIf_Types.h"

/*==================[macros]==================================================*/
/*==================[type definitions]========================================*/
/*==================[external function declarations]==========================*/

void CanIf_Init(const CanIf_ConfigType *ConfigPtr);
Std_ReturnType CanIf_Transmit(PduIdType CanTxPduId, const PduInfoType* PduInfoPtr);
void CanIf_TxConfirmation(PduIdType CanTxPduId);

/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/




#endif /* if (!defined CANIF_H) */
/*==================[end of file]=============================================*/
