/** 
 * @file        powertrain.c
 * @author      Marcel Kneib
 * @date        08.08.2013
 * @brief       Diese Datei beinhaltet die Implementierung, welche den Antrieb 
 *              umsetzt.
 * 
 * Die Implementierung umfasst die Initialisierungsroutine, welche den In- und
 * Output mittels PWM konfiguriert. Dadurch ist es möglich, die Signale der 
 * Funkfernbedienung aufnehmen und den Elektromotor ansprechen zu können.
 * 
 * Das Ansprechen wird dabei, falls die Funkfernbedienung aktiv ist, von dem
 * Task übernommen. Dieser nutzt das eingelesene Signal, um daraus Fahrt-
 * richtung und Gasstellung zu ermitteln. Diese Informationen werden
 * genutzt, um das Signal, welches an den Motor gesendet werden
 * soll, zu ermitteln. 
 * 
 * Ist die Funkfernbedienung inaktiv, so kann der Antrieb über
 * die Funktion setPWRTrainFromExtern gesteuert werden. 
 * 
 */
#include "app.h"

static void initOutput(void);
void initOC_power(void);
static void initInput(void);
void setPWRTrain(uint16_t remote_value); //was static

extern RC_STATE_T rc_state;     /**< Globale Datensturktur*/
       

void initPWRTrain() {
    initOutput();
    initInput();
}


/**
 * Initialisiert den Input.
 * PB3, TIM2_CH2, TIM2_IRQ
 */
static void initInput() {


	TIM_ICInitTypeDef TIM_ICInitStructure;
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
    TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICInitStructure.TIM_ICFilter = 0x0;
    TIM_PWMIConfig(TIM2, &TIM_ICInitStructure);

	
	/* Select the TIM4 Input Trigger: TI2FP2 */
    TIM_SelectInputTrigger(TIM2, TIM_TS_TI2FP2);

    /* Select the slave Mode: Reset Mode */
    TIM_SelectSlaveMode(TIM2, TIM_SlaveMode_Reset);
    TIM_SelectMasterSlaveMode(TIM2, TIM_MasterSlaveMode_Enable);

    /* TIM enable counter */
    TIM_Cmd(TIM2, ENABLE);

    /* Enable the CC2 Interrupt Request */
    TIM_ITConfig(TIM2, TIM_IT_CC2, ENABLE);
}

/**
 * Initialisiert den Output.
 * PB14, TIM12_CH1
 */
static void initOutput() {

    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Prescaler = 64;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = 47752 / 2;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

    TIM_TimeBaseInit(TIM12, &TIM_TimeBaseStructure);


    initOC_power();

    TIM_Cmd(TIM12, DISABLE);
    TIM_Cmd(TIM12, ENABLE);
    TIM_CtrlPWMOutputs(TIM12, DISABLE);
    TIM_CtrlPWMOutputs(TIM12, ENABLE);
	
}

void initOC_power() {
	
    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
    TIM_OCInitStructure.TIM_Pulse = PWRTRAIN_POS_NEUTRAL;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;
	
    TIM_OC1Init(TIM12, &TIM_OCInitStructure);
    TIM_OC2Init(TIM12, &TIM_OCInitStructure);	
	
}	

__IO uint32_t remote_driving_value; /**< Wert der Fernsteuerung */

	int64_t back_w = 0;
	int64_t front_w = 0;
	uint16_t taste;
	uint8_t schalter = 0;
void PWRTrainTask() {
	
	taste = sys_kldd_call(CFG_KLDD_gpio_re, 0, GPIO_A,0); 
	
	//Überprüft ob die Taste gedrückt wurde und schaltet den schalter und den grünen LED Licht ein oder aus
	if (taste == 8265 || taste == 8257 || taste == 41025|| taste == 41033) {
		sys_kldd_call(CFG_KLDD_gpio_toggle, 13, 3,0);
		sys_kldd_call(CFG_KLDD_gpio_toggle, 12, 3,0);
		if (schalter == 0) {
			schalter = 1;
		} else {
			schalter = 0;
		}
	}
	
	
	// Schaltet den langsamen Modus ein oder as
	if (schalter == 1) {
		rc_state.modus = PWRTRAIN_SLOW;
	} else {
		rc_state.modus = PWRTRAIN_FAST;
	}
	// benutzt Pden Motor 
	if (rc_state.remote == REMOTE_AKTIVE && remote_driving_value < 2000) {
			setPWRTrain(remote_driving_value);
	}

	
	sys_task_terminate();
	sys_abort();
}

float pwrtrainPercVal = 0; /**< Zwischenspeicher für das Berechnen der resultierenden Antriebsstärke*/
uint16_t speedVal;              /**< Zwischenspeicher der Antriebsstärke*/

	//Drive Mode für den Modelfahrzeug Langsam oder Schnell
	uint16_t pwrtrain_forward_max = PWRTRAIN_POS_FORWARDS_MAX;
	uint16_t pwrtrain_backwards_max = PWRTRAIN_POS_BACKWARDS_MAX;



/*
 * Wird vom Antriebs-Task verwendet und setzt die Fahrtrichtung, 
 * errechnet den Timerwert für den PWM Ausgang prüft diesen auf Plausibilität
 * und setzt den Antriebswert. 
 */
uint16_t counter = 0;
void setPWRTrain(uint16_t remote_value) {
		//vergleicht die Geschwindigkeit der beiden Räder
		back_w =  difference_back_wheels();
		front_w = difference_front_wheels();
	
    // Lenkrichtung berechnen (Zusatz falls am Distanzsensor einen Objekt ist soll es nicht rückwärts fahren.
    if (remote_value > PWRTRAIN_REMOTE_MIN_BACKWARDS && remote_value < PWRTRAIN_REMOTE_MIN_FORWARDS) {
        rc_state.driving_direction = PWRTRAIN_DIR_STOPPED;
		rc_state.wheelspeed.back_right_rotation = 0;
		rc_state.wheelspeed.back_left_rotation = 0;
		rc_state.wheelspeed.front_right_rotation = 0;
		rc_state.wheelspeed.front_left_rotation = 0;
		back_w = 0;
		front_w = 0;	
		rc_state.wheelspeed.back_right = 0;
		rc_state.wheelspeed.back_left = 0;
		rc_state.wheelspeed.front_right = 0;
		rc_state.wheelspeed.front_left = 0;
		
    } else if (remote_value > PWRTRAIN_REMOTE_MIN_FORWARDS && rc_state.front_distance == FRONT_DISTANCE_MAX) {
        rc_state.driving_direction = PWRTRAIN_DIR_FORWARDS;
    } else if (remote_value < PWRTRAIN_REMOTE_MIN_BACKWARDS && rc_state.distance_sensor == DISTANCE_NO_DETECTION) {
        rc_state.driving_direction = PWRTRAIN_DIR_BACKWARDS;
    } else {
        rc_state.driving_direction = PWRTRAIN_DIR_STOPPED;
		rc_state.wheelspeed.back_right_rotation = 0;
		rc_state.wheelspeed.back_left_rotation = 0;
		rc_state.wheelspeed.front_right_rotation = 0;
		rc_state.wheelspeed.front_left_rotation = 0;
		back_w = 0;
		front_w = 0;	
		rc_state.wheelspeed.back_right = 0;
		rc_state.wheelspeed.back_left = 0;
		rc_state.wheelspeed.front_right = 0;
		rc_state.wheelspeed.front_left = 0;
				
    }
	//Überprüfung der Geschwindigkeiten der Räder
	if ( back_w > DIFF_BACK || back_w < -DIFF_BACK || front_w > DIFF_FRONT || front_w < -DIFF_FRONT)
	{
		rc_state.driving_direction = PWRTRAIN_DIR_STOPPED;
		rc_state.wheelspeed.back_right_rotation = 0;
		rc_state.wheelspeed.back_left_rotation = 0;
		rc_state.wheelspeed.front_right_rotation = 0;
		rc_state.wheelspeed.front_left_rotation = 0;
		back_w = 0;
		front_w = 0;	
		rc_state.wheelspeed.back_right = 0;
		rc_state.wheelspeed.back_left = 0;
		rc_state.wheelspeed.front_right = 0;
		rc_state.wheelspeed.front_left = 0;
	}

	if (rc_state.modus == PWRTRAIN_SLOW) {
		pwrtrain_forward_max = PWRTRAIN_POS_FORWARDS_MAX_SLOW_MOD;
		pwrtrain_backwards_max = PWRTRAIN_POS_BACKWARDS_MAX_SLOW_MOD;	
	} else if (rc_state.modus == PWRTRAIN_FAST) {
		pwrtrain_forward_max = PWRTRAIN_POS_FORWARDS_MAX;
		pwrtrain_backwards_max = PWRTRAIN_POS_BACKWARDS_MAX;			
	}
    // Wert für den Timer berechnen
    if (rc_state.driving_direction == PWRTRAIN_DIR_STOPPED) {
        speedVal = PWRTRAIN_POS_NEUTRAL;
    } else if (rc_state.driving_direction == PWRTRAIN_DIR_BACKWARDS) {
        pwrtrainPercVal = (float) ((float) PWRTRAIN_REMOTE_MIN_MAX_DIF_BACKWARDS / (float) ((float) PWRTRAIN_REMOTE_MIN_BACKWARDS - (float) remote_value));
        speedVal = PWRTRAIN_POS_BACKWARDS_MIN - (PWRTRAIN_MIN_MAX_DIF_BACKWARDS / pwrtrainPercVal);
    } else if (rc_state.driving_direction == PWRTRAIN_DIR_FORWARDS) {
        pwrtrainPercVal = (float) ((float) PWRTRAIN_REMOTE_MIN_MAX_DIF_FORWARDS / (float) ((float) remote_value - (float) PWRTRAIN_REMOTE_MIN_FORWARDS));
        speedVal = PWRTRAIN_POS_FORWARDS_MIN + (PWRTRAIN_MIN_MAX_DIF_FORWARDS / pwrtrainPercVal);
    }

    // Plausibilitätsprüfung
    if (speedVal > pwrtrain_forward_max) {
        speedVal = pwrtrain_forward_max;
    } else if (speedVal < pwrtrain_backwards_max) {
        speedVal = pwrtrain_backwards_max;
    }
    

    TIM12->CCR1 = speedVal;
    speedVal = PWRTRAIN_POS_NEUTRAL;
}

void setPWRTrainFromExtern(uint16_t direction, uint8_t pValue) {
    
	rc_state.driving_direction = direction;

     speedVal = PWRTRAIN_POS_NEUTRAL;

    if (pValue > 100) {
        TIM12->CCR1 = speedVal;
        return;
    }

    //Prozent berechnen
    if (direction == PWRTRAIN_DIR_BRAKING) {
        if (rc_state.driving_direction == PWRTRAIN_DIR_FORWARDS) {
            TIM12->CCR1 = PWRTRAIN_POS_FORWARDS_MIN + 50;
			sys_sleep(50000000);
			speedVal = PWRTRAIN_POS_BACKWARDS_MIN - 50;
        } else {
            speedVal = PWRTRAIN_POS_NEUTRAL;
        }
    } else if (direction == PWRTRAIN_DIR_STOPPED) {
        speedVal = PWRTRAIN_POS_NEUTRAL;
    } else if (direction == PWRTRAIN_DIR_BACKWARDS) {
        speedVal = PWRTRAIN_POS_BACKWARDS_MIN - ((float) PWRTRAIN_MIN_MAX_DIF_BACKWARDS / (float) 100) * pValue;
    } else if (direction == PWRTRAIN_DIR_FORWARDS) {
        speedVal = PWRTRAIN_POS_FORWARDS_MIN + ((float) PWRTRAIN_MIN_MAX_DIF_FORWARDS / (float) 100) * pValue;
    }
	
	TIM12->CCR1 = speedVal;
		
}

/**
 * @brief Dieser Interrupt wird ausgeführt, wenn ein neuer Wert des Gashebels
 *        vorliegt und speichert diesen zwischen.
 */
void TIM2_IRQHandler(void) {
	TIM_ClearITPendingBit(TIM2, TIM_IT_CC2);
    remote_driving_value = (TIM_GetCapture1(TIM2) / 100);
	//printf("Fernbedienungwert: %i\n", remote_driving_value);	
}

