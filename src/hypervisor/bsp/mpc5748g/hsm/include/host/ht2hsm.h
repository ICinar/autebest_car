/**
* \file ht2hsm.h
*
* \brief Header file for the communication between host and the HSM.
*
* Note, the addresses are from "host side".
*
* \author easycore GmbH, 91058 Erlangen, Germany
*
* \par  License
* Customer: @@LicenseCustomer@@,
* License type: @@LicenseType@@,
* Licensed for project: @@LicenseProject@@.

* Copyright 2015 easycore GmbH. All rights exclusively reserved for easycore GmbH, 
* unless expressly agreed to otherwise.
*/

#ifndef HT2HSM_H
#define HT2HSM_H

/*==================[inclusions]==================================================================*/

/*==================[macros]======================================================================*/
/* CAUTION: "HOST SIDE" ADDRESSES */
#define HSM2HT_BASE  0xFFF30000

#define HSM2HTF_A    (HSM2HT_BASE + 0x00)
#define HSM2HTIE_A   (HSM2HT_BASE + 0x04)
#define HT2HSMF_A    (HSM2HT_BASE + 0x08)
#define HT2HSMIE_A   (HSM2HT_BASE + 0x0C)
#define HSM2HTS_A    (HSM2HT_BASE + 0x10)
#define HT2HSMS_A    (HSM2HT_BASE + 0x14)

#define HSM2HTF      *((volatile unsigned int *) HSM2HTF_A )
#define HSM2HTIE     *((volatile unsigned int *) HSM2HTIE_A)
#define HT2HSMF      *((volatile unsigned int *) HT2HSMF_A )
#define HT2HSMIE     *((volatile unsigned int *) HT2HSMIE_A)
#define HSM2HTS      *((volatile unsigned int *) HSM2HTS_A )
#define HT2HSMS      *((volatile unsigned int *) HT2HSMS_A )

/*==================[type definitions]============================================================*/

/*==================[internal function declarations]==============================================*/

/*==================[external function declarations]==============================================*/

/*==================[external constants]==========================================================*/

/*==================[internal constants]==========================================================*/

/*==================[external data]===============================================================*/

/*==================[internal data]===============================================================*/

/*==================[internal function definitions]===============================================*/

/*==================[external function definitions]===============================================*/

#endif /* !HT2HSM_H */
/*==================[end of file]=================================================================*/       
