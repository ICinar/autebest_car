#ifndef CAN_GENERATLTYPES_H


struct Can_ConfigType {
	uint8 nothing
};

struct Can_ControllerBaudrateConfigType {
	uint8 nothing
};

struct Can_PduType {
	PduIdType swPduHandle,
	uint8 length,
	Can_IdType id,
	uint8* sdu
};

#if(CAN_EXTENDED_ADDRESSING==STD_ON)
typedef uint32 Can_IdType;
#else
typedef uint16 Can_IdType;
#endif

#if(CAN_EXTENDED_HW_ADDRESSING==STD_ON)
typedef uint16 Can_HwHandleType;
#else
typedef uint8 Can_HwHandleType;
#endif

enum Can_StateTransitionType {
	CAN_T_START,
	CAN_T_STOP,
	CAN_T_SLEEP,
	CAN_T_WAKEUP
};

enum Can_ReturnType {
	CAN_OK,
	CAN_NOT_OK,
	CAN_BUSY
};

#endif