/** 
 * @file        wheelsensor.c
 * @author      Marcel Kneib
 * @date        10.08.2013
 * @brief       Enthält die Implementierung (von Marcel Kneib) der Konfiguration der Radsensoren, 
 *              die Implementierung der Interrupts sowie die Callback-Funktion
 *              des Timers, welche für das Zurücksetzen der Geschwindigkeit 
 *              genutzt wird. wurde für Autobest umgeschrieben
 */
#include "app.h"

uint64_t tmsp = 0;

int64_t speed = 0;                                       /**< Zwischenspeicher für die Berechnung der Geschwindigkeiten. */
WS_T front_left;        /**< Datenstrukturen für das Speichern der Zeitstempel für die Berechnung der Geschwindigkeiten vorne links. */
WS_T front_right;       /**< Datenstrukturen für das Speichern der Zeitstempel für die Berechnung der Geschwindigkeiten vorne rechts. */
WS_T back_left;         /**< Datenstrukturen für das Speichern der Zeitstempel für die Berechnung der Geschwindigkeiten hinten links. */
WS_T back_right;        /**< Datenstrukturen für das Speichern der Zeitstempel für die Berechnung der Geschwindigkeiten hinten rechts. */
extern RC_STATE_T rc_state;                             /**< Globale Datensturktur*/

int64_t calc_speed(uint64_t new, uint64_t old);
/*-----------------------------------------------------------------------------------*/



void initWheelSensors() {	
	
	EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;

    EXTI_InitStructure.EXTI_Line = EXTI_Line1;
    EXTI_Init(&EXTI_InitStructure);

    EXTI_InitStructure.EXTI_Line = EXTI_Line2;
    EXTI_Init(&EXTI_InitStructure);

    EXTI_InitStructure.EXTI_Line = EXTI_Line3;
    EXTI_Init(&EXTI_InitStructure);

    EXTI_InitStructure.EXTI_Line = EXTI_Line5;
    EXTI_Init(&EXTI_InitStructure);
	
	//NVIC Definition 
	
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource3);
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource1);
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource5);
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD, EXTI_PinSource2);	
	
	rc_state.wheelspeed.front_left_rotation = 0;
    front_left.timestamp = sys_gettime();
    rc_state.wheelspeed.back_left_rotation = 0;
    back_left.timestamp = sys_gettime();
	
    rc_state.wheelspeed.front_right_rotation = 0;
    front_right.timestamp = sys_gettime();
    rc_state.wheelspeed.back_right_rotation = 0;
    back_right.timestamp = sys_gettime();

	
   // xWheelsensorTimerHandle = xTimerCreate((const signed char * const)"Wheelsensor Timer", WHEELSENSOR_TIMER_PERIOD / portTICK_RATE_MS,
              //  pdTRUE, 0, vTimerWheelsensorCallback);
    
     //   xTimerStart(xWheelsensorTimerHandle, 10);
}


/**
 * @brief Gibt den aktuellen Zeitstempel.
 * @return aktuelle Zeitstempel in Nanosekunden.
 */



/**
 * @brief Prüft den Zustand des Antriebs und setzt die Geschwindigkeiten der Räder bei Stillstand zurück.
 * @param pxTimer nicht verwendet

void vTimerWheelsensorCallback(xTimerHandle pxTimer) {
    if (rc_state.driving_direction == PWRTRAIN_DIR_BRAKING || rc_state.driving_direction == PWRTRAIN_DIR_STOPPED) {
        rc_state.wheelspeed.back_left = 0;
        rc_state.wheelspeed.front_left = 0;
        rc_state.wheelspeed.back_right = 0;
        rc_state.wheelspeed.front_right = 0;
    }
}
 */
/**
 * @brief   Dieser Interrupt wird ausgelöst, wenn der vorne links angebrachte
 *          Radsensor eine Umdrehung detektiert.
 * 
 * Überprüft die Zeitspanne und berechnet die daraus resultierende Geschwindigkeit.
 * Inkrementiert die Anzahl der Umdrehungen
 */ 
uint64_t temp = 0;
uint32_t rot = 0;
void EXTI1_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line1) != RESET) {
        front_left.timestamp =sys_gettime();  
        speed = calc_speed(front_left.timestamp, front_left.timestamp_old);	
		
        if (speed >= 0) {
			rot++;
            temp = temp + speed;//            debug_print2("DIF: %d -> %f m/s (%f km/h)\n\r", front_left.timestamp - front_left.timestamp_old, rc_state.wheelspeed.front_left, rc_state.wheelspeed.front_left * 3.6);
            front_left.timestamp_old = front_left.timestamp;
        }
		if  ( rot % 10 == 0) {
            rc_state.wheelspeed.front_left_rotation++;			
			rc_state.wheelspeed.front_left = temp;	// Zeit pro 21 mm
			temp = 0;
		}
		EXTI_ClearITPendingBit(EXTI_Line1);
		
    }
}

/**
 * @brief   Dieser Interrupt wird ausgelöst, wenn der hinten links angebrachte
 *          Radsensor eine Umdrehung detektiert.
 * 
 * Überprüft die Zeitspanne und berechnet die daraus resultierende Geschwindigkeit.
 * Inkrementiert die Anzahl der Umdrehungen
 */
uint64_t temp2 = 0;
uint32_t rot2 = 0;
void EXTI2_IRQHandler(void) {
//	printf("hinten links\n");	
    if (EXTI_GetITStatus(EXTI_Line2) != RESET) {
        back_left.timestamp = sys_gettime();

        speed = calc_speed(back_left.timestamp, back_left.timestamp_old);
        if (speed >= 0) {

            temp2 = temp2 + speed;		
			rot2++;
//                        debug_print2("DIF: %d -> %f m/s (%f km/h)\n\r", back_left.timestamp - back_left.timestamp_old, rc_state.wheelspeed.back_left, rc_state.wheelspeed.back_left * 3.6);
            back_left.timestamp_old = back_left.timestamp;
        }
		if  ( rot2 % 10 == 0) {
			rc_state.wheelspeed.back_left = temp2;	// Zeit pro 21 mm
			temp2 = 0;
		    rc_state.wheelspeed.back_left_rotation++;
		}
        EXTI_ClearITPendingBit(EXTI_Line2);
    }
}

/**
 * @brief   Dieser Interrupt wird ausgelöst, wenn der vorne rechts angebrachte
 *          Radsensor eine Umdrehung detektiert.
 * 
 * Überprüft die Zeitspanne und berechnet die daraus resultierende Geschwindigkeit.
 * Inkrementiert die Anzahl der Umdrehungen
 */
uint64_t temp3 = 0;
uint32_t rot3 = 0;
void EXTI3_IRQHandler(void) {
	//printf("vorne rechts\n");	
    if (EXTI_GetITStatus(EXTI_Line3) != RESET) {
        front_right.timestamp = sys_gettime();
        speed = calc_speed(front_right.timestamp, front_right.timestamp_old);
        if (speed >= 0) {
            temp3 = temp3 + speed;			
			rot3++;
            //            debug_print2("DIF: %d -> %f m/s (%f km/h)\n\r", front_right.timestamp - front_right.timestamp_old, rc_state.wheelspeed.front_right, rc_state.wheelspeed.front_right * 3.6);
            front_right.timestamp_old = front_right.timestamp;
        }
		
		if  ( rot3 % 10 == 0) {
			rc_state.wheelspeed.front_right = temp3;	// Zeit pro 21 mm
			rc_state.wheelspeed.front_right_rotation++;
			temp3 = 0;
		}	
        EXTI_ClearITPendingBit(EXTI_Line3);
    }
}

/**
 * @brief   Dieser Interrupt wird ausgelöst, wenn der hinten rechts angebrachte
 *          Radsensor eine Umdrehung detektiert.
 * 
 * Überprüft die Zeitspanne und berechnet die daraus resultierende Geschwindigkeit.
 * Inkrementiert die Anzahl der Umdrehungen
 */
 uint64_t temp4 = 0;
uint32_t rot4 = 0;
void EXTI9_5_IRQHandler(void) {
	//printf("hinten rechts\n");	
    if (EXTI_GetITStatus(EXTI_Line5) != RESET) {
		
        back_right.timestamp = sys_gettime();
        speed = calc_speed(back_right.timestamp, back_right.timestamp_old);
        if (speed >= 0) {
            temp4 = temp4 + speed;		
			rot4++;
      
            //            debug_print2("DIF: %d -> %f m/s (%f km/h)\n\r", front_right.timestamp - front_right.timestamp_old, rc_state.wheelspeed.front_right, rc_state.wheelspeed.front_right * 3.6);
            back_right.timestamp_old = front_right.timestamp;
        }
		
		if  ( rot4 % 10 == 0) {
		       rc_state.wheelspeed.back_right_rotation++;	// 1 Umdrehung
			rc_state.wheelspeed.back_right = temp4;			// Zeit pro 21 mm
			temp4 = 0;
		}
        EXTI_ClearITPendingBit(EXTI_Line5);
    }
}

/**
 * @brief Überprüft die vergangene Zeit seit des letzten Interrupts und berechnet
 *        daraus mithilfe des Reifenumfangs die resultierende Geschwindigkeit.
 * @param new Neuer Zeitstempel
 * @param old Alter Zeitstempel
 * @return -1, wenn die minimale Zeitspanne nicht überschritten wurde, sonst die
 *      daraus resultierende Geschwindigkeit.
 */
inline int64_t calc_speed(uint64_t new, uint64_t old) {
    if (new - old > TIME_DIF) {
        //return (double) TIRE_PERIMETER / (double) (new - old);
        return (new - old);
    } else {
        return -1;
    }
} 

/** @brief Überprüft die Beiden Radsensorwerte am vorderen Rädern
 *		   sobald die beiden Rädern mehr als fün Prozent beträgt soll er den 
 *		   Fahrzeug stoppen. 
 *@param speedl Geschwindigkeit der vordere Rad links
 *@param speedr Geschwindigkeit der vordere Rad rechts
 *
*/
int64_t difference_front_wheels() {
	int64_t diff_front;
	//differenz berechnen
	diff_front = rc_state.wheelspeed.front_right - rc_state.wheelspeed.front_left;
	return diff_front;
}


/** @brief Überprüft die Beiden Radsensorwerte am hinteren Rädern
 *		   sobald die beiden Rädern mehr als fün Prozent beträgt soll er den 
 *		   Fahrzeug stoppen. 
 *@param speedl Geschwindigkeit der hintere Rad links
 *@param speedr Geschwindigkeit der hintere Rad rechts
 *
 */
int64_t difference_back_wheels() {
	int64_t diff_back = 0;
	//differenz berechnen
	diff_back = rc_state.wheelspeed.back_right - rc_state.wheelspeed.back_left;
	//printf("BACKKKK: %i\n", diff_back);
	return diff_back;
} 	

