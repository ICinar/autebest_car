/**
 * \file      Std_Types.h
 * \brief     AUTOSAR standard types.
 * \details   This AUTOSAR mandatory header contains all types that are used
 *            across several modules of the basic software and that are platform
 *            and compiler independent.
 *
 * \see       http://www.autosar.org/fileadmin/files/releases/4-2/software-architecture/general/standard/AUTOSAR_SWS_StandardTypes.pdf
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

#if (!defined STD_TYPES_H)
#define STD_TYPES_H

/*==================[inclusions]==============================================*/

#include <Compiler.h>
#include <Platform_Types.h>

/*==================[macros]==================================================*/

#ifndef STATUSTYPEDEFINED
#define STATUSTYPEDEFINED
#ifndef E_OK
#define E_OK        0x00u
#endif
typedef unsigned char StatusType;   /* OSEK compliance */
#endif
#define E_NOT_OK    0x01u

#define STD_HIGH    0x01u   /* Physical state 5V or 3.3V */
#define STD_LOW     0x00u   /* Physical state 0V */

#define STD_ACTIVE  0x01u   /* Logical state active */
#define STD_IDLE    0x00u   /* Logical state idle */

#define STD_ON      0x01u
#define STD_OFF     0x00u

/*==================[type definitions]========================================*/

/* boolean */
typedef unsigned char   boolean;

typedef uint8 Std_ReturnType;

typedef struct
{
    uint16  vendorID;
    uint16  moduleID;
    uint8   sw_major_version;
    uint8   sw_minor_version;
    uint8   sw_patch_version;
} Std_VersionInfoType;

/*==================[external function declarations]==========================*/
/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/

#endif /* if (!defined STD_TYPES_H) */
/*==================[end of file]=============================================*/
