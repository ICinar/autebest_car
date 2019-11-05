/**
 * \file      ComStack_Types.h
 * \brief     Communication stack types.
 * \details   Conforming AUTOSAR 4.2
 *
 * \see       http://www.autosar.org/fileadmin/files/releases/4-2/software-architecture/communication-stack/standard/AUTOSAR_SWS_CommunicationStackTypes.pdf
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

#if (!defined COMSTACK_TYPES_H)
#define COMSTACK_TYPES_H

/*==================[inclusions]==============================================*/

#include "Std_Types.h"
#include "ComStack_Cfg.h"

/*==================[macros]==================================================*/
/*==================[type definitions]========================================*/

/* SWS_COMTYPE_00011 */
typedef struct
{
  uint8*        SduDataPtr;
  PduLengthType SduLength;
} PduInfoType;

/* SWS_COMTYPE_00031
 * Specify the parameter to which the value has to be changed (BS or STmin).
 */
typedef enum
{
    TP_STMIN = 0x00, /* Separation Time */
    TP_BS    = 0x01, /* Block Size */
    TP_BC    = 0x02, /* The Band width control parameter used in FlexRay
                      * transport protocol module. */
} TPParameterType;

/* SWS_COMTYPE_00012 
 * Variables of this type shall be used to store the result of a buffer request.
 */
typedef enum
{
    /* Buffer  request  accomplished  successful. */
    BUFREQ_OK       = 0,
    /* Buffer request not successful. Buffer cannot be accessed. */
    BUFREQ_E_NOT_OK = 1,
    BUFREQ_NOT_OK   = 1, /* Non AUTOSAR, used by ArcCore*/
    /* Temporarily no buffer available. It's up the requester to retry
     * request  for  a  certain  time. */
    BUFREQ_E_BUSY   = 2,
    BUFREQ_BUSY     = 2, /* Non AUTOSAR, used by ArcCore*/
    /* No Buffer of the required length can be provided. */
    BUFREQ_E_OVFL   = 3,
    BUFREQ_OVFL     = 3, /* Non AUTOSAR, used by ArcCore*/
} BufReq_ReturnType;

/* SWS_COMTYPE_00038 */
typedef uint8 NetworkHandleType;



/* SWS_COMTYPE_00027 */
typedef enum
{
    TP_DATACONF,
    TP_DATARETRY,
    TP_CONFPENDING,
    TP_NORETRY,
} TpDataStateType;

/* SWS_COMTYPE_00037 */
typedef struct
{
    TpDataStateType TpDataState;
    PduLengthType   TxTpDataCnt;
} RetryInfoType;

/*------------------[ARS 4.0 Types]-------------------------------------------*/
/* These types have been removed in AUTOSAR 4.1.
 * Definitions taken from R4.0 Rev 3.
 */

typedef uint8 NotifResultType;

/* COMTYPE018 */
#define NTFRSLT_OK                     0x00
#define NTFRSLT_E_NOT_OK               0x01
#define NTFRSLT_E_TIMEOUT_A            0x02
#define NTFRSLT_E_TIMEOUT_BS           0x03
#define NTFRSLT_E_TIMEOUT_CR           0x04
#define NTFRSLT_E_WRONG_SN             0x05
#define NTFRSLT_E_INVALID_FS           0x06
#define NTFRSLT_E_UNEXP_PDU            0x07
#define NTFRSLT_E_WFT_OVRN             0x08
#define NTFRSLT_E_ABORT                0x09
#define NTFRSLT_E_NO_BUFFER            0x0A
#define NTFRSLT_E_CANCELATION_OK       0x0B
#define NTFRSLT_E_CANCELATION_NOT_OK   0x0C
#define NTFRSLT_PARAMETER_OK           0x0D
#define NTFRSLT_E_PARAMETER_NOT_OK     0x0E
#define NTFRSLT_E_RX_ON                0x0F
#define NTFRSLT_E_VALUE_NOT_OK         0x10

/*==================[external function declarations]==========================*/
/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/

#endif /* if (!defined COMSTACK_TYPES_H) */
/*==================[end of file]=============================================*/
