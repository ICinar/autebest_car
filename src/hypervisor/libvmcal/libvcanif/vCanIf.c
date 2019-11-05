/**
 * \file     CanIf.c
 * \brief    Dummy implementation of CanIf functions.
 *
 * \date     16.11.2015
 * \author   Liviu Beraru <Liviu.Beraru@easycore.com>
 * \author   easycore GmbH, 91058 Erlangen, Germany
 *
 * \par          License
 * Customer:     @@LicenseCustomer@@,
 * License type: @@LicenseType@@,
 * Licensed for project: @@LicenseProject@@.
 *
 * \copyright Copyright 2015 easycore GmbH, 91058 Erlangen, Germany.
 * All rights exclusively reserved for easycore GmbH, unless expressly agreed
 * to otherwise.
 */

/*==================[inclusions]==============================================*/

#include <vCanIf.h>
#include <vCanIf_Cbk.h>
#include "vCan.h"

#include <hv.h>
#include <Std_Types.h> /* Std_ReturnType */

/*==================[macros]==================================================*/
/*==================[type definitions]========================================*/
/*==================[external function declarations]==========================*/
/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/

static CanIf_ConfigType CanIf_Config;
static uint32 CanIf_Initialized = 0;

/*==================[external function definitions]===========================*/

/*------------------[Initialization]------------------------------------------*/

void CanIf_Init(const CanIf_ConfigType *ConfigPtr)
{
    if (ConfigPtr != NULL_PTR)
    {
        /* copy configuration data */
        CanIf_Config.vCanIf_Transmit_Entries_Count = ConfigPtr->vCanIf_Transmit_Entries_Count;
        CanIf_Config.vCanIf_Transmit_Entries       = ConfigPtr->vCanIf_Transmit_Entries;

        CanIf_Config.vCanIf_RxIndication_Entries_Count = ConfigPtr->vCanIf_RxIndication_Entries_Count;
        CanIf_Config.vCanIf_RxIndication_Entries       = ConfigPtr->vCanIf_RxIndication_Entries;

        CanIf_Config.vCanIf_TxConfirmation_Entries_Count = ConfigPtr->vCanIf_TxConfirmation_Entries_Count;
        CanIf_Config.vCanIf_TxConfirmation_Entries       = ConfigPtr->vCanIf_TxConfirmation_Entries;
        
        CanIf_Initialized = 1;
    }
    else
    {
        CanIf_Initialized = 0;
    }
}

/*------------------[Request for Transmission]--------------------------------*/

Std_ReturnType CanIf_Transmit(PduIdType CanTxPduId, const PduInfoType* PduInfoPtr)
{
    static uint8       data[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    static Can_PduType PduInfo = { .sdu = data };

    CanIf_Transmit_Config* message_box = NULL_PTR;
    Std_ReturnType retVal = E_NOT_OK;
    size_t i = 0;

    if (CanIf_Initialized == 1)
    {
        if (CanTxPduId < CanIf_Config.vCanIf_Transmit_Entries_Count)
        {
            message_box = &( CanIf_Config.vCanIf_Transmit_Entries[CanTxPduId] );
            if (message_box->pdu != NULL_PTR)
            {
                retVal = E_OK;

                PduInfo.swPduHandle = message_box->pdu->swPduHandle;
                PduInfo.length      = message_box->pdu->length;
                PduInfo.id          = message_box->pdu->id;

                for (i = 0; i < PduInfo.length; i++)
                {
                    PduInfo.sdu[i] = message_box->pdu->sdu[i];
                }

                Can_Write(message_box->pdu->MessageBoxID, &PduInfo);
            }
        }        
    }

    (void) PduInfoPtr;

    return retVal;
}

/*------------------[Callback notifications]----------------------------------*/

void CanIf_RxIndication
(
    Can_HwHandleType Hrh, /* Message box number */
    Can_IdType       CanId,
    uint8            CanDlc,
    const uint8*     CanSduPtr
)
{
    size_t i = 0;
    size_t j = 0;
    CanIf_RxIndication_Config* message_box = NULL_PTR;

    if (CanIf_Initialized == 1)
    {
        for (i = 0; i < CanIf_Config.vCanIf_RxIndication_Entries_Count; i++)
        {
            message_box = &( CanIf_Config.vCanIf_RxIndication_Entries[i] );

            if ((message_box != NULL_PTR) && (message_box->pdu != NULL_PTR))
            {
                message_box->pdu->MessageBoxID = Hrh;
                message_box->pdu->swPduHandle  = CanId; /* FIXME: PDU ID = ??? */
                message_box->pdu->id           = CanId;
                message_box->pdu->length       = CanDlc;

                for (j = 0u; j < CanDlc; j++)
                {
                    message_box->pdu->sdu[j] = CanSduPtr[j];
                }

                sys_ipev_set(message_box->ipev);
            }
        }
    }
}

void CanIf_TxConfirmation(PduIdType CanTxPduId)
{
    CanIf_TxConfirmation_Config* message_box = NULL_PTR;

    if (CanIf_Initialized == 1)
    {
        if (CanTxPduId < CanIf_Config.vCanIf_TxConfirmation_Entries_Count)
        {
            message_box = &( CanIf_Config.vCanIf_TxConfirmation_Entries[CanTxPduId] );
            if ((message_box != NULL_PTR) && (message_box->pduid != NULL_PTR))
            {
                *(message_box->pduid) = CanTxPduId;
                /* set tx confirmation event on transmission partition */
                sys_ipev_set(message_box->ipev);
            }
        }
    }
}

void CanIf_ControllerBusOff(uint8 ControllerId)
{
    (void)ControllerId;
}

void CanIf_ControllerModeIndication(uint8 ControllerId, CanIf_ControllerModeType ControllerMode)
{
    (void)ControllerId;
    (void)ControllerMode;
}


/*==================[internal function definitions]===========================*/
/*==================[end of file]=============================================*/
