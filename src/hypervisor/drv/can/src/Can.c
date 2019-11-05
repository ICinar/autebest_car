#include <Can_GeneralTypes.h>

void Can_Init(const Can_ConfigType* Config)
{
 /* DO NOTHING */
}

void Can_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
 /* DO NOTHING */
}

Std_ReturnType Can_CheckBaudrate(uint8 Controller, const uint16 Baudrate)
{
	return E_NOT_OK;
}

Std_ReturnType Can_ChangeBaudrate(uint8 Controller, const uint16 Baudrate)
{
	return E_NOT_OK;
}

Can_ReturnType Can_SetControllerMode(uint8 Controller, Can_StateTransitionType Transition) 
{
	return CAN_NOT_OK;
}

void Can_DisableControllerInterrupts(uint8 Controller) 
{
 /* DO NOTHING */
}

void Can_EnableControllerInterrupts(uint8 Controller)
{
 /* DO NOTHING */
}

Can_ReturnType Can_CheckWakeup(uint8 Controller)
{
	return CAN_NOT_OK;
}

Can_ReturnType Can_Write(Can_HwHandleType Hth, const Can_PduType* PduInfo)
{
	return CAN_NOT_OK;
}

void Can_MainFunction_Write(void)
{
 /* DO NOTHING */
}

void Can_MainFunction_Read(void)
{
 /* DO NOTHING */
}

void Can_MainFunction_BusOff(void)
{
 /* DO NOTHING */
}

void Can_MainFunction_Wakeup(void)
{
 /* DO NOTHING */
}

void Can_MainFunction_Mode(void)
{
 /* DO NOTHING */
}