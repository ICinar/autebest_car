/** 
 * @file        distancesensor.c
 * @author      Marcel Kneib
 * @date        10.09.2013
 * @brief       Enthält die Konfiguration des Distanzsensors und die 
 *              Implementierung des Interrupts.
 */

#include "app.h"

extern RC_STATE_T rc_state;     /**< Globale Datensturktur*/

void initDistanceSensor() {



    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;

    EXTI_InitStructure.EXTI_Line = EXTI_Line15;
    EXTI_Init(&EXTI_InitStructure);
	
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource15);

//    EXTI_GenerateSWInterrupt(EXTI_Line15);

}

/**
 * @brief   Dieser Interrupt wird ausgelöst, wenn der Distanzsensor seinen
 *          Status ändert.
 * 
 * Überprüft den aktuellen Zustand und aktualisiert die Datenstruktur.
 * Liegt eine 0 an, so erkennt der Sensor ein Objekt - bei einer 1, wird nichts
 * erkannt.
 *  
 */

void EXTI15_10_IRQHandler(void) {
	if (EXTI_GetITStatus(EXTI_Line15) != RESET) {
		uint16_t test = sys_kldd_call(CFG_KLDD_gpio_re, 15, GPIO_A,0); 
		printf("DISTANZSENSOR: %i\n", test);
		if (test > 40000) {
            rc_state.distance_sensor = DISTANCE_DETECTION;
			printf("distance detected\n");			
        } else {
            rc_state.distance_sensor = DISTANCE_NO_DETECTION;
			printf("DISTANZ not detected\n");			
        }    
        EXTI_ClearITPendingBit(EXTI_Line15);          
    }
}
