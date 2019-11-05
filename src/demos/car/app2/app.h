/*
 * app.h
 *
 * Initial user space program, partition1
 *
 * azuepke, 2013-12-06: initial
 * azuepke, 2014-05-03: renamed to app.h
 */


#include <stdint.h>
#include <stdio.h>
#include <hv_sys.h>
#include <hv_error.h>
#include <assert.h>
#include <hv_types.h>
#include "stm32f4xx_syscfg.h"
#include "stm32f4xx_exti_bsp.h"
#include "stm32f4_discovery_lis302dl_bsp.h"  

#ifndef __APP_H__
#define __APP_H__

/* generated IDs */
#include "app.id.h"


#define GPIO_A									0
#define GPIO_B									1
#define GPIO_C									2
#define GPIO_D									3
#define GPIO_E									4

#define ACTIVE_COUNTER_START            		5

/*----------------------------------------------ACCELEROMETER-------------------------------------*/
     /**
     * @brief   Diese Funktion initialisiert den Beschleunigungssensor.
     * 
     * Als Danach wird der Interrupt (EXTI0_IRQn) des Sensors aktiviert. wird der LIS302DL aktiviert, worauf eine kurze Pause folgen 
     * muss und die interne Liste initialisiert wird. 
     * 
     * Danach wird der Interrupt (EXTI0_IRQn) des Sensors aktiviert.
     *  
     */
    void initAccelerometer(void);
	void EXTI0_IRQHandler(void);



/*----------------------------------------GENERAL--------------------------------------------*/

    /**
     * @brief   Dieses Struct beinhaltet die Werte des Beschleunigungssensors 
     *          in mg.
     */
    typedef struct {
        volatile uint8_t x; /**< Beschleunigung in Richtung der X-Achse. */
        volatile uint8_t y; /**< Beschleunigung in Richtung der Y-Achse. */
        volatile uint8_t z; /**< Beschleunigung in Richtung der Z-Achse. */
    } ACCELEROMETER_T;
    /**
     * @brief   Dieses Struct beinhaltet alle Informationen, welche vom System 
     *          ermittelt werden.
     * 
     * Es beinhaltet die aktuelle Lenk- und Fahrrichtung, den Status der Fern-
     * steuerung, die Beschleunigungswerte, die Raddrehzahlen, die Signalleuchten,
     * die gemessene Entfernung eines vor dem Fahrzeug befindlichen Objektes, das
     * Kamerabild sowie der Zustand des Distanzsensors an der rechten Seite.
     * 
     */
    typedef struct {
        ACCELEROMETER_T accelerometer;                  /**< Beschleunigungswerte */
    } RC_STATE_T;




/* main.c */
void _start(void);
void main(void);
void acc_irq_handler(void);

/*accelerometer */
void AccelerometerTask(void);



/* buildid.c -- created at build time */
extern const char __buildid[];


#endif
