/** 
 * @file        steering.c
 * @author      Marcel Kneib
 * @date        10.08.2013
 * @brief       Diese Datei beinhaltet die Implementierung, welche die Lenkung 
 *              umsetzt.
 * 
 * Die Implementierung umfasst die Initialisierungsroutine, welche den In- und
 * Output mittels PWM konfiguriert. Dadurch ist es möglich, die Signale der 
 * Funkfernbedienung aufnehmen und den Lenkservo ansprechen zu können.
 * 
 * Das Ansprechen wird dabei, falls die Funkfernbedienung aktiv ist, von dem
 * Task übernommen. Dieser nutzt das eingelesene Signal, um daraus Lenk-
 * richtung und Ausschlag zu ermitteln. Diese Informationen werden
 * genutzt, um das Signal, welches an den Servo gesendet werden
 * soll, zu ermitteln. 
 * 
 * Ist die Funkfernbedienung inaktiv, so kann die Lenkung über
 * die Funktion setStreeringFromExtern gesteuert werden. Bei Deaktivierung
 * der Fernbedienung wird das Fahrzeug gestoppt, der Antriebs-Task angehalten
 * und der autonome Task gestartet.
 */

#include "app.h"


static void initOutput(void);
static void initOC(void);
static void initInput(void);
static void initIC(TIM_TypeDef* TIMx);
static void setSteering(uint16_t remote_value); 
extern RC_STATE_T rc_state; /**< Globale Datensturktur*/
__IO uint32_t remote_steering_value; /**< Über den Interrupt ermittelter Wert der Funk-Fernbedienung, welcher für die Steuerung der Lenkung verwendet wird. */
uint8_t steering_activity_counter = 0; /**< Aktivitätszähler für das Feststellen des Zustands der Funk-Fernbedienung. */

//xTaskHandle xPWRTrainHandle; /**< Tasks Handle für das Staten und Stoppen des Antrieb*/
//xTaskHandle xCommunicationHandle; /**< Tasks Handle für das Starten und Stoppen des autonomen Task. */

void initSteering() {	
	initOutput(); 
    initInput();
    //xPWRTrainHandle = getPWRTrainHandle();
    //xCommunicationHandle = getCommunicationHandle();
}

/**
 * Initialisiert den Output.
 * PB15, TIM12_CH2
 */
void initOutput() {

	uint16_t TimerPeriod = 47752 / 2; //(SystemCoreClock /1);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Prescaler = 64;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = TimerPeriod;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

	
    TIM_TimeBaseInit(TIM12, &TIM_TimeBaseStructure);
    initOC();


    TIM_Cmd(TIM12, DISABLE);
    TIM_Cmd(TIM12, ENABLE);
    TIM_CtrlPWMOutputs(TIM12, DISABLE);
    TIM_CtrlPWMOutputs(TIM12, ENABLE);
	
}

void initOC() {

    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
    TIM_OCInitStructure.TIM_Pulse = STEERING_POS_NEUTRAL / 2;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;	
    TIM_OC1Init(TIM12, &TIM_OCInitStructure);
    TIM_OC2Init(TIM12, &TIM_OCInitStructure);
}


/**
 * Initialisiert den Input.
 * PB4, TIM3_CH1, TIM3_IRQn
 */
void initInput() {
	

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Prescaler = 32;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = 0xffff;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

	
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);  
	


    initIC(TIM3);


}

void initIC(TIM_TypeDef* TIMx) {

    TIM_ICInitTypeDef TIM_ICInitStructure;
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
    TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICInitStructure.TIM_ICFilter = 0x0;
    TIM_PWMIConfig(TIMx, &TIM_ICInitStructure);   
	TIM_SelectInputTrigger(TIMx, TIM_TS_TI1FP1);

    TIM_SelectSlaveMode(TIMx, TIM_SlaveMode_Reset);
    TIM_SelectMasterSlaveMode(TIMx, TIM_MasterSlaveMode_Enable);

    TIM_Cmd(TIMx, ENABLE);
    TIM_ITConfig(TIMx, TIM_IT_CC1, ENABLE);
	
}


void SteeringTask(void) {
	setSteering(STEERING_REMOTE_MID);	  
	switch(rc_state.remote) {
		case REMOTE_AKTIVE:		
			if (steering_activity_counter <= 0) {
				// Zeit abgelaufen, Fernsteuerung wieder inaktiv setzen
        		setPWRTrainFromExtern(PWRTRAIN_DIR_BRAKING, 0);
          		setRemoteLight(LIGHT_OFF);
          		rc_state.remote = REMOTE_INAKTIVE;

			}else{
        		rc_state.remote = REMOTE_AKTIVE;
          		steering_activity_counter--;
          		setSteering(remote_steering_value);
			}
		break;
		
		case REMOTE_INAKTIVE:

			if(steering_activity_counter == ACTIVE_COUNTER_START){ // Erneute Fernsteuerung registeriert
        		setPWRTrainFromExtern(PWRTRAIN_DIR_BRAKING, 0);
          		rc_state.remote = REMOTE_AKTIVE;
          		setRemoteLight(LIGHT_ON);

			}
		break;
		}		
	
	sys_task_terminate();
	sys_abort(); 	
}


/*
void vSteeringTask(void *pvParameters) {

	const portTickType xDelay = TASK_STEERING_PERIOD / portTICK_RATE_MS;
  portTickType xLastWakeTime = xTaskGetTickCount();
  setStreering(STEERING_REMOTE_MID);

  for (;;) {
        
  	switch(rc_state.remote){
			case REMOTE_AKTIVE: 
            
				if(steering_activity_counter <= 0) { // Zeit abgelaufen, Fernsteuerung wieder inaktiv setzen
        	setPWRTrainFromExtern(PWRTRAIN_DIR_BRAKING, 0);
          setRemoteLight(LIGHT_OFF);
          rc_state.remote = REMOTE_INAKTIVE;
                
					vTaskSuspend(xPWRTrainHandle);
          vTaskResume(xCommunicationHandle);
				}else{
        	rc_state.remote = REMOTE_AKTIVE;
          steering_activity_counter--;
          setStreering(remote_steering_value);
				}
				break;

			case REMOTE_INAKTIVE:
      	if(steering_activity_counter == ACTIVE_COUNTER_START){ // Erneute Fernsteuerung registeriert
        	setPWRTrainFromExtern(PWRTRAIN_DIR_BRAKING, 0);
          rc_state.remote = REMOTE_AKTIVE;
          setRemoteLight(LIGHT_ON);

					vTaskSuspend(xCommunicationHandle);
          vTaskResume(xPWRTrainHandle);
				}
				break;
		}		
		vTaskDelayUntil(&xLastWakeTime, xDelay);
	}

}
  */        



float steeringPercVal = 0;     /**< Zwischenspeicher für das Berechnen des resultierenden Lenkeinschlags*/
uint16_t steerVal;              /**< Zwischenspeicher des Lenkeinschlags*/
/*
 * Wird vom Steuerungs-Task verwendet und setzt die Lenkrichtung, 
 * errechnet den Timerwert für den PWM Ausgang prüft diesen auf Plausibilität
 * und setzt den Lenkwert. 
 */
void setSteering(uint16_t remote_value) {	
     steerVal = STEERING_POS_NEUTRAL;


    if (remote_value > STEERING_REMOTE_MIN_LEFT && remote_value < STEERING_REMOTE_MIN_RIGHT) {
        rc_state.steering_direction = STEERING_DIR_NEUTRAL;	
    } else if (remote_value > STEERING_REMOTE_MIN_RIGHT) {
				printf("Lenkung rechts\n");		
		rc_state.steering_direction = STEERING_DIR_RIGHT;
    } else if (remote_value < STEERING_REMOTE_MIN_LEFT) {
				printf("Lenkung links\n"); 
		rc_state.steering_direction = STEERING_DIR_LEFT;
    }

	

    // Wert für den Timer berechnen
    if (rc_state.steering_direction == STEERING_DIR_NEUTRAL) {
        steerVal = STEERING_POS_NEUTRAL;
    } else if (rc_state.steering_direction == STEERING_DIR_LEFT) {
        steeringPercVal = (float) ((float) STEERING_REMOTE_MIN_MAX_DIF_LEFT / (float) ((float) STEERING_REMOTE_MIN_LEFT - (float) remote_value));
        steerVal = STEERING_POS_NEUTRAL - (STEERING_MIN_MAX_DIF_LEFT / steeringPercVal);
    } else if (rc_state.steering_direction == STEERING_DIR_RIGHT) {
        steeringPercVal = (float) ((float) STEERING_REMOTE_MIN_MAX_DIF_RIGHT / (float) ((float) remote_value - (float) STEERING_REMOTE_MIN_RIGHT));
        steerVal = STEERING_POS_NEUTRAL + (STEERING_MIN_MAX_DIF_RIGHT / steeringPercVal);
    } else if ( rc_state.steering_direction == STEERING_DIR_RIGHT && rc_state.distance_sensor == DISTANCE_NO_DETECTION) {
        steerVal = STEERING_POS_NEUTRAL;		
	}
	
	
	
		

    if (steerVal < STEERING_POS_LEFT_MAX) {
        steerVal = STEERING_POS_LEFT_MAX;
    } else if (steerVal > STEERING_POS_RIGHT_MAX) {
        steerVal = STEERING_POS_RIGHT_MAX;
    }
    
    TIM12->CCR2 = steerVal / 2;
}

void setStreeringFromExtern(uint16_t direction, uint8_t pValue) {
	rc_state.steering_direction = direction;

    steerVal = STEERING_POS_NEUTRAL;

    if (pValue > 100) {
        TIM12->CCR2 = steerVal / 2;
        return;
    }

    //Prozent berechnen
    if (direction == STEERING_DIR_NEUTRAL) {
        steerVal = STEERING_POS_NEUTRAL;
    } else if (direction == STEERING_DIR_RIGHT) {
        steerVal = STEERING_POS_NEUTRAL + (float) (STEERING_MIN_MAX_DIF_RIGHT / 100) * pValue;
    } else if (direction == STEERING_DIR_LEFT) {
        steerVal = STEERING_POS_NEUTRAL - (float) (STEERING_MIN_MAX_DIF_LEFT / 100) * pValue;
    }

    
    TIM12->CCR2 = steerVal / 2;
}

/**
 * @brief Dieser Interrupt wird ausgeführt, wenn ein neuer Wert der Lenkung
 *        vorliegt und speichert diesen zwischen. Weiterhin wird der Aktivitäts-
 *        zähler zurückgesetzt.
 */
void TIM3_IRQHandler(void) { 

	TIM_ClearITPendingBit(TIM3, TIM_IT_CC1);
    remote_steering_value = (TIM_GetCapture2(TIM3));
    steering_activity_counter = ACTIVE_COUNTER_START;
}
