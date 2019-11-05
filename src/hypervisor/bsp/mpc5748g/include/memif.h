/**
 * \file      memif.h
 * \brief     Interface of  memory interface module.
 * \details
 *
 * \date      03.12.2015
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

#if (!defined MEMIF_H)
#define MEMIF_H

/*==================[inclusions]==============================================*/
/*==================[macros]==================================================*/

/*==================[type definitions]========================================*/
/*==================[external function declarations]==========================*/

/**
 * \brief Initialize PRAMC and PFLASH.
 */
void board_config_memory(void);

/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/

#endif /* if (!defined MEMIF_H) */
/*==================[end of file]=============================================*/

