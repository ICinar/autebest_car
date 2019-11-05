/**
 * \file     Can.c
 * \brief    Dummy implementation of the Autosar CAN module.
 * \details
 *
 * \date     19.11.2015
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

#include <vCan.h>
#include <vCanIf_Cbk.h>
#include "SHM_Serialized_Types.h"

#include <hv_sys.h> /* kernel types like addr_t, size_t */

/*==================[macros]==================================================*/
/*==================[type definitions]========================================*/
/*==================[external function declarations]==========================*/
/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/

static Can_ConfigType   vCan_Config;
static uint32 Can_Initialized = 0;

/*==================[external function definitions]===========================*/

void Can_Init(const Can_ConfigType* Config)
{
    if (Config != NULL_PTR)
    {
        vCan_Config.vCan_Write_Entries_Count = Config->vCan_Write_Entries_Count;
        vCan_Config.vCan_Write_Entries       = Config->vCan_Write_Entries;

        vCan_Config.vCan_RxISR_Entries_Count = Config->vCan_RxISR_Entries_Count;
        vCan_Config.vCan_RxISR_Entries       = Config->vCan_RxISR_Entries;

        vCan_Config.vCanTx_Confirmation_shm  = Config->vCanTx_Confirmation_shm;
        
        Can_Initialized = 1;
    }
    else
    {
        Can_Initialized = 0;
    }
}

Can_ReturnType Can_Write(Can_HwHandleType Hth, const Can_PduType* PduInfo)
{
    Can_ReturnType retVal     = CAN_NOT_OK;
    Can_Write_Config* message_box = NULL_PTR;
    size_t client             = 0;
    size_t data_idx           = 0;

    if (Can_Initialized == 1) /* see SWS_Can_00216, ASR 4.2 */
    {
        for (client = 0u; client < vCan_Config.vCan_Write_Entries_Count; client++)
        {
            message_box = &( vCan_Config.vCan_Write_Entries[client] );
            retVal     = CAN_NOT_OK;

            if ((message_box != NULL_PTR) && (message_box->pdu != NULL_PTR))
            {
                message_box->pdu->MessageBoxID= Hth;
                message_box->pdu->swPduHandle = PduInfo->swPduHandle;
                message_box->pdu->length      = PduInfo->length;
                message_box->pdu->id          = PduInfo->id;

                for (data_idx = 0u; data_idx < PduInfo->length; data_idx++)
                {
                    message_box->pdu->sdu[data_idx] = PduInfo->sdu[data_idx];
                }
                
                *(message_box->pduid) = PduInfo->swPduHandle;

                /* set interpartition event, which activates the trasmission task
                 * which delegates the message to the real can driver */
                sys_ipev_set(message_box->ipev);
                
                retVal = CAN_OK;
            }
        }
    }

    return retVal;
}

/**
 * \brief   A CAN message has been received.
 * \details CAN reception ISR.
 *
 * - Hardware receives message and generates interrupt.
 * - The interrupt service routine calls on the CAN partition the function
 *   CanIf_RxIndication of the vCanIf library.
 * - The vCanIf lib copies the received information into shared memory and
 *   triggers an interpartition event.
 * - The ipev wakes a user task on the user partition.
 * - The user task calls this function which pushes the information into the
 *   AUTOSAR stack by calling CanIf_RxIndication.
 *
 * Note that CanIf_RxIndication being called by this function is not the one of
 * the vCanIf library, but the real one of the AUTOSAR stack.
 */

void CAN_RX_ISR(void)
{
    Can_RX_ISR_Config* message_box = NULL_PTR;

    if (Can_Initialized == 1)
    {
        if (vCan_Config.vCan_RxISR_Entries_Count > 0)
        {
            /* Handle only the first entry */
            message_box = &( vCan_Config.vCan_RxISR_Entries[0] );
            
            if ((message_box != NULL_PTR) && (message_box->pdu != NULL_PTR))
            {
                /* Push received data up into the AUTOSAR stack */
                CanIf_RxIndication
                (
                    message_box->pdu->MessageBoxID,
                    message_box->pdu->id,
                    message_box->pdu->length,
                    message_box->pdu->sdu
                );
            }
        }
    }
}

void CAN_TX_Confirmation(void)
{
    if ((Can_Initialized == 1) && (vCan_Config.vCanTx_Confirmation_shm != NULL_PTR))
    {
        CanIf_TxConfirmation(* vCan_Config.vCanTx_Confirmation_shm);
    }
}

void Can_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    (void)versioninfo;
}

Std_ReturnType Can_CheckBaudrate(uint8 Controller, const uint16 Baudrate)
{
    (void)Controller;
    (void)Baudrate;
    return E_NOT_OK;
}

Std_ReturnType Can_ChangeBaudrate(uint8 Controller, const uint16 Baudrate)
{
    (void)Controller;
    (void)Baudrate;
    return E_NOT_OK;
}

Can_ReturnType Can_SetControllerMode(uint8 Controller, Can_StateTransitionType Transition)
{
    (void)Controller;
    (void)Transition;
    return CAN_OK;
}

void Can_DisableControllerInterrupts(uint8 Controller)
{
    /* DO NOTHING */
    (void)Controller;
}

void Can_EnableControllerInterrupts(uint8 Controller)
{
    /* DO NOTHING */
    (void)Controller;
}

Can_ReturnType Can_CheckWakeup(uint8 Controller)
{
    (void)Controller;
    return CAN_OK;
}

#if (!defined Can_MainFunction_Write)
void Can_MainFunction_Write(void)
{
    /* DO NOTHING */
}
#endif

#if (!defined Can_MainFunction_Read)
void Can_MainFunction_Read(void)
{
    /* DO NOTHING */
}
#endif

#if (!defined Can_MainFunction_BusOff)
void Can_MainFunction_BusOff(void)
{
    /* DO NOTHING */
}
#endif

#if (!defined Can_MainFunction_Wakeup)
void Can_MainFunction_Wakeup(void)
{
    /* DO NOTHING */
}
#endif

void Can_MainFunction_Mode(void)
{
    /* DO NOTHING */
}

/*==================[internal function definitions]===========================*/
/*==================[end of file]=============================================*/
