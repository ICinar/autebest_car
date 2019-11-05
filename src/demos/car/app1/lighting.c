/** 
 * @file        lighting.c
 * @author      Marcel Kneib
 * @date        02.09.2013
 * @brief       Enthält Konfiguration, die Funktionen für das Ansteuern der
 *              Signalleuchten sowie der Timer, welcher für das Blinken
 *              genutzt wird.
 */

#include <hv.h>					//kernel include
#include <stdio.h>
#include <linker.h>				//kernel include
#include <hv_compiler.h>		//kernel include		
#include <assert.h>				//kernel include
#include <hv_error.h>	
#include <string.h>
#include "app.h"
void checkTimer(void);
RC_STATE_T rc_state;     /**< Globale Datensturktur*/
//xTimerHandle xLightTimerHandle; /**< Timer Handle für das Togglen der LEDs*/


void initLights() {
   /* xLightTimerHandle = xTimerCreate((const signed char* const)"Light Timer", LIGHT_TIMER_PERIOD / portTICK_RATE_MS,
            pdTRUE, 0, vTimerLightingCallback); */
}

void setStopLights(uint8_t state) {
    if (state == LIGHT_ON ) {
	   rc_state.lights.stop = LIGHT_ON;
       sys_kldd_call(CFG_KLDD_gpio_res, 8, 2, 0);
    } else if (state == LIGHT_OFF) {
        rc_state.lights.stop = state;
       sys_kldd_call(CFG_KLDD_gpio_s, 8, 2, 0);
    }
    
}

void setBlinkerLights(uint8_t state) {
    if (state == BLINKER_LIGHTS_LEFT_ACTIVE) {
        rc_state.lights.blinker_left = LIGHT_ON;
    } else if (state == BLINKER_LIGHTS_RIGHT_ACTIVE) {
        rc_state.lights.blinker_right = LIGHT_ON;
    } else if (state == BLINKER_LIGHTS_INACTIVE) {
        rc_state.lights.blinker_left = LIGHT_OFF;
        rc_state.lights.blinker_right = LIGHT_OFF;
        sys_kldd_call(CFG_KLDD_gpio_s, 9, 2, 0);
        sys_kldd_call(CFG_KLDD_gpio_s, 11, 2, 0);
    }
    checkTimer();
}

void setRemoteLight(uint8_t state) {
    if (state == LIGHT_ON) {
        rc_state.lights.remote = LIGHT_ON;
    } else if (state == LIGHT_OFF) {
		sys_kldd_call(CFG_KLDD_gpio_s, 15, 3, 0);
        rc_state.lights.remote = LIGHT_OFF;
    }
    checkTimer();
}

/*
 * Prüft ob das Togglen der Leuchten notwendig ist und startet bzw. stoppt 
 * anschließend den Timer.
 */
void checkTimer() {
	printf("HIER BIN ICH LIGHTING\n");
 	if (rc_state.lights.blinker_left == LIGHT_ON) {
		sys_kldd_call(CFG_KLDD_gpio_toggle,9,GPIO_C,0);
		sys_sleep(10000000);
	}
	if (rc_state.lights.blinker_right == LIGHT_ON) {
		sys_kldd_call(CFG_KLDD_gpio_toggle,11,GPIO_C,0);
		sys_sleep(10000000);
	}
    if (rc_state.lights.remote == LIGHT_ON){
		sys_kldd_call(CFG_KLDD_gpio_toggle,15,3,0);
		sys_sleep(10000000);		
	}
}


