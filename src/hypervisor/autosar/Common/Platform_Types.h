/**
 * \file      Platform_Types.h
 * \brief     Platform types.
 * \details   This AUTOSAR mandatory file contains platform dependent types
 *            and symbols.
 *
 * \see       http://www.autosar.org/fileadmin/files/releases/4-2/software-architecture/implementation-integration/standard/AUTOSAR_SWS_PlatformTypes.pdf
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

#if (!defined PLATFORM_TYPES_H)
#define PLATFORM_TYPES_H

/*==================[inclusions]==============================================*/
/*==================[macros]==================================================*/


#ifndef TRUE
#define TRUE            1u
#endif

#ifndef FALSE
#define FALSE           0u
#endif

#ifndef true
#define true  1u
#endif
#ifndef false
#define false 0u
#endif

/* CPU types, bit format and endianess */
#define CPU_TYPE_8      8
#define CPU_TYPE_16     16
#define CPU_TYPE_32     32

#define MSB_FIRST       0
#define LSB_FIRST       1

#define HIGH_BYTE_FIRST 0
#define LOW_BYTE_FIRST  1


/* settings for standard 32-bit CPUs like ARM and PowerPC */
#define CPU_TYPE        CPU_TYPE_32

#if (defined __powerpc__) || (defined __PPC__)
#define CPU_BIT_ORDER   MSB_FIRST
#else
#define CPU_BIT_ORDER   LSB_FIRST
#endif

#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) /* GCC specific */
#define CPU_BYTE_ORDER  HIGH_BYTE_FIRST
#else
#define CPU_BYTE_ORDER  LOW_BYTE_FIRST
#endif

/*==================[type definitions]========================================*/

/* unsigned base types */
typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned long       uint32;
typedef unsigned long long  uint64;

typedef unsigned long       uint8_least;
typedef unsigned long       uint16_least;
typedef unsigned long       uint32_least;


/* signed base types */
typedef signed char         sint8;
typedef signed short        sint16;
typedef signed long         sint32;
typedef signed long long    sint64;

typedef signed long         sint8_least;
typedef signed long         sint16_least;
typedef signed long         sint32_least;


/* floating point types */
typedef float               float32;
typedef double              float64;



/*==================[external function declarations]==========================*/
/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/

#endif /* if (!defined PLATFORM_TYPES_H) */
/*==================[end of file]=============================================*/
