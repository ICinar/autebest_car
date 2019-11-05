/** 
 * @file        ultrasonic.c
 * @author      Marcel Kneib
 * @date        2.09.2013
 * @brief       Enthält die Implementierung der Initialisierungsroutine für
 *              den Ultraschallsensor sowie den Interrupt, welcher von
 *              diesem ausgelöst wird. In diesem wird der Entfernungswert
 *              berechnet und in der globalen Datenstruktur abgelegt.
 */
#include "app.h"

extern RC_STATE_T rc_state; /**< Globale Datensturktur*/
TIM_ICInitTypeDef TIM_ICInitStructure;

/**
 * Initialisiert den Ultraschallsensor.
 * PA8, TIM1_CH1
 */

void initUltrasonic() {

	//Timer Initialization
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Prescaler = 64;	
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = 0xffff;	
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;		
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;	
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
	
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
    TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICInitStructure.TIM_ICFilter = 0x0;
    TIM_PWMIConfig(TIM1, &TIM_ICInitStructure);	
    TIM_SelectInputTrigger(TIM1, TIM_TS_TI1FP1);
    TIM_UpdateRequestConfig(TIM1, TIM_UpdateSource_Regular);
    TIM_SelectSlaveMode(TIM1, TIM_SlaveMode_Reset);
    TIM_SelectMasterSlaveMode(TIM1, TIM_MasterSlaveMode_Enable);	
    TIM_Cmd(TIM1, ENABLE);
    TIM_ITConfig(TIM1, TIM_IT_CC1, ENABLE);	
	
}



/**
 * @brief Dieser Interrupt wird ausgeführt, wenn ein neuer Wert des Ultraschall-
 *        sensors vorliegt. Anhand dieses Wertes wird die entsprechende 
 *        Entfernung berechnet und in der Datenstruktur gespeichert.
 */
void TIM1_CC_IRQHandler(void) {
	if (TIM_GetFlagStatus(TIM1, TIM_IT_CC1) == SET) {
        TIM_ClearITPendingBit(TIM1, TIM_IT_CC1);
        //rc_state.front_distance = (double) ((double) ((double) (TIM_GetCapture2(TIM1) * 0.381)) / 147 * 2.54); /7 Berechnung nun auf Pi
        rc_state.front_distance = TIM_GetCapture2(TIM1);
    }
	printf(" Meter %i\n", rc_state.front_distance);
}
