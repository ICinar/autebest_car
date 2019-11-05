/**
 * \file      vCan.h
 * \brief     Declaration of the Can module API.
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

#if (!defined VCAN_H)
#define VCAN_H

/*==================[inclusions]==============================================*/

#include <ComStack_Types.h>
#include <vCan_GeneralTypes.h>

/*==================[macros]==================================================*/
/*==================[type definitions]========================================*/

/* Types are defined in Can_GeneralTypes.h */

/*==================[external function declarations]==========================*/

extern void Can_Init(const Can_ConfigType* Config);
extern void Can_GetVersionInfo(Std_VersionInfoType* versioninfo);
extern Std_ReturnType Can_CheckBaudrate(uint8 Controller, const uint16 Baudrate);
extern Std_ReturnType Can_ChangeBaudrate(uint8 Controller, const uint16 Baudrate);
extern Can_ReturnType Can_SetControllerMode(uint8 Controller, Can_StateTransitionType Transition);
extern void Can_DisableControllerInterrupts(uint8 Controller);
extern void Can_EnableControllerInterrupts(uint8 Controller);
extern Can_ReturnType Can_CheckWakeup(uint8 Controller);
extern Can_ReturnType Can_Write(Can_HwHandleType Hth, const Can_PduType* PduInfo);

#if (!defined Can_MainFunction_Write)
extern void Can_MainFunction_Write(void);
#endif

#if (!defined Can_MainFunction_Read)
extern void Can_MainFunction_Read(void);
#endif

#if (!defined Can_MainFunction_BusOff)
extern void Can_MainFunction_BusOff(void);
#endif

#if (!defined Can_MainFunction_Wakeup)
extern void Can_MainFunction_Wakeup(void);
#endif

extern void Can_MainFunction_Mode(void);

/* Hardware informs the CAN driver about received message */
void CAN_RX_ISR(void);

/* Process transmission confirmation */
void CAN_TX_Confirmation(void);

/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/

#endif /* if (!defined VCAN_H) */
/*==================[end of file]=============================================*/
