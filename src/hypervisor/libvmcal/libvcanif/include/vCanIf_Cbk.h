/**
 * \file      CanIf_Cbk.h
 * \brief     Callback functions called by the CAN module.
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

#if (!defined CANIF_CBK_H)
#define CANIF_CBK_H

/*==================[inclusions]==============================================*/

/* Inclusion not specified but necessary due to otherwise missing types */
#include <vCanIf_Types.h>

/*==================[macros]==================================================*/
/*==================[type definitions]========================================*/
/*==================[external function declarations]==========================*/

void CanIf_TxConfirmation(PduIdType CanTxPduId);
void CanIf_RxIndication(Can_HwHandleType Hrh, Can_IdType CanId, uint8 CanDlc, const uint8 *CanSduPtr);
void CanIf_CancelTxConfirmation(PduIdType CanTxPduId, const Can_PduType *PduInfoPtr);
void CanIf_ControllerBusOff(uint8 ControllerId);
void CanIf_ControllerModeIndication(uint8 ControllerId, CanIf_ControllerModeType ControllerMode);

/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/


#endif /* if (!defined CANIF_CBK_H) */
/*==================[end of file]=============================================*/
