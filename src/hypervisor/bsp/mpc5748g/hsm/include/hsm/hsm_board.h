/**
* \file hsm_board.h
*
* \brief
*
* \details
*
* \author easycore GmbH, 91058 Erlangen, Germany
*
* \version 
*
* \par  License
* Customer: @@LicenseCustomer@@,
* License type: @@LicenseType@@,
* Licensed for project: @@LicenseProject@@.

* Copyright 2016 easycore GmbH, All rights exclusively reserved for easycore GmbH,
* unless expressly agreed to otherwise.
*/

#ifndef HSM_BOARD_H
#define HSM_BOARD_H

/*==================[inclusions]==================================================================*/

/*==================[macros]======================================================================*/

/*==================[type definitions]============================================================*/

/*==================[internal function declarations]==============================================*/

/*==================[external function declarations]==============================================*/
/* -----------------------------------------------------------------------------------------------*/
/**
 * \brief  Resets the board.
 */
/* -----------------------------------------------------------------------------------------------*/
void hsm_board_reset
(
   void
);

/* -----------------------------------------------------------------------------------------------*/
/**
 * \brief  Get the current debug status of the HSM.
 *
 * \param result Pointer to the result of the debug request. If the result is 1, there is a lock
 * request for Main application debug. Otherwise, there is an unlock request.
 */
/* -----------------------------------------------------------------------------------------------*/
void hsm_read_mdur
(
   unsigned char *result
);

/* -----------------------------------------------------------------------------------------------*/
/**
 * \brief  Toggles the MDUR bit of the HSM control register.
 */
/* -----------------------------------------------------------------------------------------------*/
void hsm_toggle_mdur
(
   void
);

/*==================[external constants]==========================================================*/

/*==================[internal constants]==========================================================*/

/*==================[external data]===============================================================*/

/*==================[internal data]===============================================================*/

/*==================[internal function definitions]===============================================*/

/*==================[external function definitions]===============================================*/

#endif /* !HSM_BOARD_H */
/*==================[end of file]=================================================================*/       
