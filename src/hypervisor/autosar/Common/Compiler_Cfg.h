/**
 * \file    Compiler_Cfg.h
 * \brief   Compiler Configuration.
 * \details AUTOSAR mandatory header.
 *
 * Based on AUTOSAR 4.2.2, "Specification of Compiler Abstraction"
 * 
 * \date    18.11.2015
 * \see http://www.autosar.org/fileadmin/files/releases/4-2/software-architecture/implementation-integration/standard/AUTOSAR_SWS_CompilerAbstraction.pdf
 *
 *
 * \author    easycore GmbH, 91058 Erlangen, Germany
 * \copyright Copyright 2015 easycore GmbH.
 * All rights exclusively reserved for easycore GmbH, unless expressly agreed
 * to otherwise.
 *
 */

#if (!defined COMPILER_CFG_H)
#define COMPILER_CFG_H
/*==================[inclusions]==============================================*/
/*==================[macros]==================================================*/

/*------------------[CAN memory and pointer classes]--------------------------*/

#define CAN_CODE
#define CAN_VAR
#define CAN_CONST
#define CAN_APPL_DATA
#define CAN_APPL_CONST

/*------------------[RTE memory and pointer classes]--------------------------*/

#define RTE_CODE

/*------------------[COM memory and pointer classes]--------------------------*/

#define COM_APPL_CODE

/*------------------[CSM/CRY memory and pointer classes]----------------------*/
#define CSM_APPL_CODE
#define CSM_APPL_DATA
#define CRY_APPL_DATA
#define CSM_CODE
#define CSM_VAR

#if (defined CSM_CONST)
#error CSM_CONST already defined
#endif
#define CSM_CONST

/*------------------[CAL memory and pointer classes]--------------------------*/

#if (defined CAL_CODE)
#error CAL_CODE already defined
#endif
#define CAL_CODE

#if (defined CAL_CONST)
#error CAL_CONST already defined
#endif
#define CAL_CONST

#if (defined CAL_APPL_DATA)
#error CAL_APPL_DATA already defined
#endif
#define CAL_APPL_DATA

#if (defined CAL_APPL_CONST)
#error CAL_APPL_CONST already defined
#endif
#define CAL_APPL_CONST

#if (defined CAL_APPL_CODE)
#error CAL_APPL_CODE already defined
#endif
#define CAL_APPL_CODE

#if (defined CAL_VAR_NOINIT)
#error CAL_VAR_NOINIT already defined
#endif
#define CAL_VAR_NOINIT

#if (defined CAL_VAR_POWER_ON_INIT)
#error CAL_VAR_POWER_ON_INIT already defined
#endif
#define CAL_VAR_POWER_ON_INIT

#if (defined CAL_VAR_FAST)
#error CAL_VAR_FAST already defined
#endif
#define CAL_VAR_FAST

#if (defined CAL_VAR)
#error CAL_VAR already defined
#endif
#define CAL_VAR

/*------------------[SecOC memory and pointer classes]------------------------*/

#define SECOC_APPL_CODE
#define SECOC_CONST
#define SECOC_VAR

/*==================[type definitions]========================================*/
/*==================[external function declarations]==========================*/
/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/

#endif /* if (!defined COMPILER_CFG_H) */
/*==================[end of file]=============================================*/
