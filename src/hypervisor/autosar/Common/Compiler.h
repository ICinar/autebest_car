/**
 * \file
 *
 * \brief   Implementation of AUTOSAR Compiler Abstraction.
 * \details Macros for the abstraction of compiler specific keywords used for
 *          addressing data and code within declarations and definitions.
 *
 * \see     http://www.autosar.org/fileadmin/files/releases/4-2/software-architecture/implementation-integration/standard/AUTOSAR_SWS_CompilerAbstraction.pdf
 * \author easycore GmbH, 91058 Erlangen, Germany
 *
 * \par License
 * Customer: @@LicenseCustomer@@,
 * License type: @@LicenseType@@,
 * Licensed for project: @@LicenseProject@@.
 *
 * Copyright 2013 easycore GmbH
 * All rights exclusively reserved for easycore GmbH, unless expressly agreed to otherwise.
 */
#if (!defined COMPILER_H)
#define COMPILER_H

/*==================[inclusions]=================================================================*/
#include "Compiler_Cfg.h"

/*==================[macros]=====================================================================*/
/*------------------[storage classes]------------------------------------------------------------*/

#if (defined AUTOMATIC)
#error AUTOMATIC already defined
#endif

/* COMPILER046 */
/** \brief definition of an automatic memory class
 **
 ** To be used for local non static variables */
#define AUTOMATIC

#if (defined TYPEDEF)
#error TYPEDEF already defined
#endif

/* COMPILER059 */
/** \brief definition of an type-definition memory class
 **
 ** To be used within type definitions only */
#define TYPEDEF

#if (defined STATIC)
#error STATIC already defined
#endif

/** \brief abstraction of compiler keyword 'static
 **
 ** values: 'static' or empty */
#define STATIC static

#if (defined _STATIC_)
#error _STATIC_ already defined
#endif

/** \brief map _STATIC_ to STATIC for Autosar 2.1 backward compatibility */
#define _STATIC_ STATIC

/*------------------[null pointer constant]------------------------------------------------------*/

#if (defined NULL_PTR)
#error NULL_PTR already defined
#endif

/* COMPILER051 */
/** \brief abstraction of the null pointer constant */
#define NULL_PTR ((void *)0)

/**
 * \brief Null pointer constant.
 * \note  The NULL constant is NOT specified by the AUTOSAR standard and the
 * user should use NULL_PTR instead. The reason we define it is because some
 * implementations like ArcticCore use it extensively.
 * Also note that the NULL macro is defined in stddef.h according to C99.
 */
#if (!defined NULL)
#define NULL (NULL_PTR)
#endif

/*------------------[storage classes]------------------------------------------------------------*/

#if (defined INLINE)
#error INLINE already defined
#endif

/* COMPILER057 */
/** \brief definition of an inline keyword
 **
 ** To be used for inlining functions */
#define INLINE

#if (defined _INLINE_)
#error _INLINE_ already defined
#endif

/** \brief map _INLINE_ to INLINE for Autosar 2.1 backward compatibility */
#define _INLINE_ INLINE

#if (defined LOCAL_INLINE)
#error LOCAL_INLINE is already defined
#endif

/** \brief Definition of a keyword for 'static inline' functions */
#define LOCAL_INLINE

/*------------------[macros for functions]-------------------------------------------------------*/

#if (defined FUNC)
#error FUNC already defined
#endif

/* COMPILER001 */
/** \brief abstraction for function declaration and definition
 **
 ** This macro abstracts the declaration and definition of functions
 ** and ensures the correct syntax of function declaration as
 ** required by the specific compiler.
 **
 ** \param[in] rettype return type of the function
 ** \param[in] memclass classification of the function itself */
#define FUNC(rettype, memclass) rettype memclass

#if (defined FUNC_P2CONST)
#error FUNC_P2CONST already defined
#endif

/* COMPILER061 */
/** \brief abstraction for function declaration and definition
 **
 ** This macro abstracts the declaration and definition of functions
 ** returning a pointer to a constant and ensures the correct syntax
 ** of function declarations as required by a specific compiler.
 **
 ** \param[in] rettype return type of the function
 ** \param[in] ptrclass defines the classification of the pointers distance
 ** \param[in] memclass classification of the function itself */
#define FUNC_P2CONST(rettype, ptrclass, memclass) const rettype * memclass

#if (defined FUNC_P2VAR)
#error FUNC_P2VAR already defined
#endif

/* COMPILER063 */
/** \brief abstraction for function declaration and definition
 **
 ** This macro abstracts the declaration and definition of functions
 ** returning a pointer to a variable and ensures the correct syntax
 ** of function declarations as required by a specific compiler.
 **
 ** \param[in] rettype return type of the function
 ** \param[in] ptrclass defines the classification of the pointers distance
 ** \param[in] memclass classification of the function itself */
#define FUNC_P2VAR(rettype, ptrclass, memclass) rettype * memclass

/*------------------[macros for pointers]--------------------------------------------------------*/

#if (defined P2VAR)
#error P2VAR already defined
#endif

/* COMPILER006 */
/** \brief abstraction for pointers in RAM pointing to RAM
 **
 ** This macro abstracts the declaration and definition of pointers
 ** in RAM pointing to variables in RAM.
 **
 ** The pointer itself is modifiable.
 ** The pointer's target is modifiable.
 **
 ** \param[in] ptrtype type of the referenced variable
 ** \param[in] memclass classification of the pointer's variable itself
 ** \param[in] defines the classification of the pointer's distance */
#define P2VAR(ptrtype, memclass, ptrclass) ptrtype * memclass ptrclass

#if (defined P2CONST)
#error P2CONST already defined
#endif

/* COMPILER013 */
/** \brief abstraction for pointers in RAM pointing to ROM
 **
 ** This macro abstracts the declaration and definition of pointers
 ** in RAM pointing to constants in ROM.
 **
 ** The pointer itself is modifiable.
 ** The pointer's target is not modifiable (read only).
 **
 ** \param[in] ptrtype type of the referenced constant
 ** \param[in] memclass classification of the pointer's variable itself
 ** \param[in] defines the classification of the pointer's distance */
#define P2CONST(ptrtype, memclass, ptrclass) ptrtype const * memclass ptrclass

#if (defined CONSTP2VAR)
#error CONSTP2VAR already defined
#endif

/* COMPILER031 */
/** \brief abstraction for pointers in ROM pointing to RAM
 **
 ** This macro abstracts the declaration and definition of pointers
 ** in ROM pointing to variables in RAM.
 **
 ** The pointer is not modifiable. (read only).
 ** The pointer's target is modifiable.
 **
 ** \param[in] ptrtype type of the referenced variable
 ** \param[in] memclass classification of the pointer's variable itself
 ** \param[in] defines the classification of the pointer's distance */
#define CONSTP2VAR(ptrtype, memclass, ptrclass) ptrtype * const memclass ptrclass

#if (defined CONSTP2CONST)
#error CONSTP2CONST already defined
#endif

/* COMPILER032 */
/** \brief abstraction for pointers in ROM pointing to ROM
 **
 ** This macro abstracts the declaration and definition of pointers
 ** in ROM pointing to constants in ROM.
 **
 ** The pointer itself is not modifiable (read only).
 ** The pointer's target is not modifiable (read only).
 **
 ** \param[in] ptrtype type of the referenced constant
 ** \param[in] memclass classification of the pointer's variable itself
 ** \param[in] defines the classification of the pointer's distance */
#define CONSTP2CONST(ptrtype, memclass, ptrclass) ptrtype const * const memclass ptrclass

/* COMPILER039 */
#if (defined P2FUNC)
#error P2FUNC already defined
#endif

/** \brief abstraction for declaration and definition of function pointers
 **
 ** This macro abstracts the declaration and definition of pointers
 ** to functions.
 **
 ** \param[in] rettype return type of the function
 ** \param[in] ptrclass defines the classification of the pointer's distance
 ** \param[in] function name respectively name of the defined type
 ** */
#define P2FUNC(rettype, ptrclass, fctname) rettype (* fctname) ptrclass

/*------------------[keywords for constants]-----------------------------------------------------*/

/* the next check is a workaround for the cygwin definition
*  of CONST in w32api/windef.h */
#if (defined CONST)
#undef CONST
#endif

/* COMPILER023 */
/** \brief abstraction for declaration and definition of constants
 **
 ** This macro abstracts the declaration and definition of constants.
 **
 ** \param[in] consttype type of the constant
 ** \param[in] memclass classification of the constant itself */
#define CONST(consttype, memclass) consttype const memclass

/*------------------[keywords for variables]-----------------------------------------------------*/

/* COMPILER026 */
#if (defined VAR)
#error VAR already defined
#endif

/** \brief abstraction for the declaration and definition of variables
 **
 ** This macro abstracts the declaration and definition of variables.
 **
 ** \param[in] vartype type of the variable
 ** \param[in] memclass classification of the variable itself */
#define VAR(vartype, memclass) vartype memclass

/*==================[type definitions]===========================================================*/
/*==================[external function declarations]=============================================*/
/*==================[internal function declarations]=============================================*/
/*==================[external constants]=========================================================*/
/*==================[internal constants]=========================================================*/
/*==================[external data]==============================================================*/
/*==================[internal data]==============================================================*/

#endif /* if !defined( COMPILER_H ) */
/*==================[end of file]================================================================*/

