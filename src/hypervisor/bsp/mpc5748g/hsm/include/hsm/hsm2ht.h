/**
* \file hsm2ht.h
*
* \brief Header file for the communication between the HSM and the host.
*
* Note, the addresses are from "HSM side".
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

#ifndef HSM2HT_H
#define HSM2HT_H

/*==================[inclusions]==================================================================*/

/*==================[macros]======================================================================*/
/* CAUTION: "HSM SIDE" ADDRESSES */
#define HSM2HT_BASE		0xA3F88000

#define HSM2HTF_A    (HSM2HT_BASE + 0x00)
#define HSM2HTIE_A   (HSM2HT_BASE + 0x04)
#define HT2HSMF_A    (HSM2HT_BASE + 0x08)
#define HT2HSMIE_A   (HSM2HT_BASE + 0x0C)
#define HSM2HTS_A    (HSM2HT_BASE + 0x10)
#define HT2HSMS_A    (HSM2HT_BASE + 0x14)

#define HSM2HTF      *((volatile unsigned int *) HSM2HTF_A )
#define HSM2HTIE	   *((volatile unsigned int *) HSM2HTIE_A)
#define HT2HSMF      *((volatile unsigned int *) HT2HSMF_A )
#define HT2HSMIE     *((volatile unsigned int *) HT2HSMIE_A)
#define HSM2HTS      *((volatile unsigned int *) HSM2HTS_A )
#define HT2HSMS      *((volatile unsigned int *) HT2HSMS_A )


#define HSBI_BCR     *((volatile unsigned int *) (0xA3F14000 + 0x00))
#define HSBI_CCR     *((volatile unsigned int *) (0xA3F14000 + 0x04))

#define SWT_HSM_CR   *((volatile unsigned int *) (0xFFF2C000 + 0x00))
#define SWT_HSM_SR   *((volatile unsigned int *) (0xFFF2C000 + 0x10))


/*==================[type definitions]============================================================*/

/*==================[internal function declarations]==============================================*/

/*==================[external function declarations]==============================================*/
extern void hsm_main(void);
extern unsigned int hsm_execute_cmd(unsigned int, void *);

/*==================[external constants]==========================================================*/

/*==================[internal constants]==========================================================*/

/*==================[external data]===============================================================*/

/*==================[internal data]===============================================================*/

/*==================[internal function definitions]===============================================*/

/*==================[external function definitions]===============================================*/

#endif /* !HSM2HT_H */
