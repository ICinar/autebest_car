/**
 * \file
 *
 * \brief easycore Crypto Library
 *
 * This file contains common types used by all modules.
 *
 * \author Bernhard Jungk, easycore GmbH, 91058 Erlangen, Germany
 *
 * \par License
 * Customer: @@LicenseCustomer@@,
 * License type: @@LicenseType@@,
 * Licensed for project: @@LicenseProject@@.
 *
 * Copyright 2014 easycore GmbH
 * All rights exclusively reserved for easycore GmbH, unless expressly agreed to otherwise.
 */

#ifndef TYPES_CFG_H
#define TYPES_CFG_H

/*==================[inclusions]============================================*/

/*==================[macros]================================================*/

#define EC_CRYPT_ON   1u
#define EC_CRYPT_OFF  0u

#define TRUE 1
#define FALSE 0

/*==================[type definitions]======================================*/


typedef unsigned long  uint8_least;
typedef unsigned int  uint16_least;

typedef signed char    sint8;

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned long  uint32;
typedef unsigned long long uint64;
typedef unsigned int   boolean;


/*==================[external function declarations]========================*/
/*==================[internal function declarations]=========================*/
/*==================[external constants]=====================================*/
/*==================[internal constants]=====================================*/
/*==================[external data]==========================================*/
/*==================[internal data]==========================================*/
/*==================[external function definitions]==========================*/
/*==================[internal function definitions]==========================*/

#endif /* ifndef TYPES_CFG_H */

/*==================[end of file]===========================================*/