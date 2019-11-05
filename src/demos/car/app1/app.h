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
#include "stm32f4xx_exti_bsp.h"
#include "stm32f4xx_tim_bsp.h"
#include "stm32f4xx_syscfg.h"



#ifndef __APP_H__
#define __APP_H__

/* generated IDs */
#include "app.id.h"

#define WHEELSENSOR_TIMER_PERIOD 250            /**< Die Perdiode des Timers, welcher auf Stillstand der Räder prüft. (in ms) */
#define TIRE_PERIMETER 2100                      /**< Umfang der Reifen für die Ermittlung der Geschwindigkeit. (in mm *10) */
#define TIME_DIF 25                           /**< Dieser Wert wird für das Akzeptieren einer Umdrehung verwendet. (in ms) */


#define REMOTE_INAKTIVE                 0       /**< Fernsteuerung ausgeschaltet. */
#define REMOTE_AKTIVE                   1       /**< Fernsteuerung eingeschaltet. */

#define LIGHT_TIMER_PERIOD                      500     /**< Periode des Timers*/

#define LIGHT_OFF                               0       /**< Leuchte aus */
#define LIGHT_ON                                1       /**< Leuchte an */

#define BLINKER_LIGHTS_INACTIVE                 0       /**< Blinker aus */
#define BLINKER_LIGHTS_LEFT_ACTIVE              1       /**< linken Blinker anschalten */
#define BLINKER_LIGHTS_RIGHT_ACTIVE             2       /**< rechten Blinker anschalten */

#define REMOTE_LIGHT_INACTIVE                   0       /**< Remote Licht aus */
#define REMOTE_LIGHT_ACTIVE                     1       /**< Remote Licht an */


#define MG_PER_DIGIT_2G  19.8   /**< mg pro Wert für +-2g */
#define MG_PER_DIGIT_8G  79.2   /**< mg pro Wert für +-8g */
#define MG_MIDDLE_VALUE  2535   /**< 2^8 * MG pro Einheit / 2 */
#define MG_MAX_VALUE     5049   /**< Max Wert des Sensors */
#define DISTANCE_DETECTION 1                    /**< Dieser Wert wird in der globalen Datenstruktur abgelegt, wenn ein Objekt erkannt wurde. */
#define DISTANCE_NO_DETECTION 0                 /**< Keine Erkennung eines Objektes. */

#define GPIO_A									0
#define GPIO_B									1
#define GPIO_C									2
#define GPIO_D									3
#define GPIO_E									4


#define PWRTRAIN_DIR_STOPPED                    0       /**< Antrieb angehalten */
#define PWRTRAIN_DIR_FORWARDS                   1       /**< Antrieb forwärts */
#define PWRTRAIN_DIR_BACKWARDS                  2       /**< Antrieb rückwärts */
#define PWRTRAIN_DIR_BRAKING                    3       /**< bremsend */

#define PWRTRAIN_POS_FORWARDS_MAX_SLOW_MOD		2015
#define	PWRTRAIN_POS_BACKWARDS_MAX_SLOW_MOD		1790

#define PWRTRAIN_SLOW 1
#define PWRTRAIN_FAST 0

#define FRONT_DISTANCE_MAX	0
//#define FRONT_DISTANCE_MAX 1500

#define DIFF_FRONT 1000000000000				/*vordere Radsensoren Toleranzwert */
#define DIFF_BACK  1000000000000				/*hintere Radsensoren Toleranzwert */				

#define PWRTRAIN_POS_NEUTRAL                    1900//1900    /**< PWM Signal für die neutrale Lenkstellung. */
#define PWRTRAIN_POS_FORWARDS_MIN               2025    /**< PWM Signal für die minimale Forwärtsfahrt. */
#define PWRTRAIN_POS_FORWARDS_MAX               2080//2740    /**< PWM Signal für die maximale Forwärtsfahrt. */
#define PWRTRAIN_POS_BACKWARDS_MIN              1740    /**< PWM Signal für die minimale Rückwärtsfahrt. */
#define PWRTRAIN_POS_BACKWARDS_MAX              1700//1060    /**< PWM Signal für die maximale Rückwärtsfahrt. */
#define PWRTRAIN_MIN_MAX_DIF_FORWARDS           55//750     /**< Differenz zwischen dem maximalen und minimalen Signal während der Forwärtsfahrt. */
#define PWRTRAIN_MIN_MAX_DIF_BACKWARDS          40//740     /**< Differenz zwischen dem maximalen und minimalen Signal während der Rückwärtsfahrt. */

#define PWRTRAIN_REMOTE_MID                     1310   /**< Signal des Gashebels in Nullstellung. */
#define PWRTRAIN_REMOTE_MAX_FORWARDS            1650   /**< Grenze des Signals des Gashebel für die Forwärtsfahrt. */
#define PWRTRAIN_REMOTE_MIN_FORWARDS            1350   /**< Ab diesem Signal des Gashebel fährt das Fahrzeug vorwärts. */
#define PWRTRAIN_REMOTE_MIN_MAX_DIF_FORWARDS    300    /**< Differenz zwischen dem maximalen und minimalen Signal des Gashebels während der Forwärtsfahrt. */
#define PWRTRAIN_REMOTE_MAX_BACKWARDS           780    /**< Grenze des Signals des Gashebel für die Rückwärtsfahrt. */
#define PWRTRAIN_REMOTE_MIN_BACKWARDS           1270   /**< Ab diesem Signal des Gashebel fährt das Fahrzeug rückwärts. */
#define PWRTRAIN_REMOTE_MIN_MAX_DIF_BACKWARDS   490    /**< Differenz zwischen dem maximalen und minimalen Signal des Gashebels während der Forwärtsfahrt. */
#define STEERING_DIR_NEUTRAL            0       /**< Lenkung nicht eingeschlagen. */
#define STEERING_DIR_LEFT               1       /**< Lenkung nach links eingeschlagen. */
#define STEERING_DIR_RIGHT              2       /**< Lenkung nach rechts eingeschlagen. */

#define STEERING_POS_LEFT_MAX           2400    /**< PWM Signal für den maximalen Linkseinschlag. */
#define STEERING_POS_RIGHT_MAX          4800    /**< PWM Signal für den maximalen rechtseinschlag. */
#define STEERING_MIN_MAX_DIF_RIGHT      ((STEERING_POS_RIGHT_MAX-STEERING_POS_LEFT_MAX)/2)     /**< Differenz zwischen des maximalen und minimalen PWM Signal für den Rechtseinschlag. */
#define STEERING_MIN_MAX_DIF_LEFT       ((STEERING_POS_RIGHT_MAX-STEERING_POS_LEFT_MAX)/2)     /**< Differenz zwischen des maximalen und minimalen PWM Signal für den Linkseinschlag. */
#define STEERING_POS_NEUTRAL            (STEERING_POS_LEFT_MAX+((STEERING_MIN_MAX_DIF_LEFT+STEERING_MIN_MAX_DIF_RIGHT)/2))    /**< PWM Signal für die neutrale Lenkstellung. */

#define STEERING_REMOTE_MID                     3600
#define STEERING_BUFFER_TOLERANCE								600
#define STEERING_REMOTE_MAX_LEFT                2400   /**< Grenze des Signals des Gashebel für die Forwärtsfahrt. */
#define STEERING_REMOTE_MAX_RIGHT               4800   /**< Grenze des Signals des Gashebel für die Rückwärtsfahrt. */
#define STEERING_REMOTE_MIN_LEFT                STEERING_REMOTE_MID - STEERING_BUFFER_TOLERANCE   /**< Ab diesem Signal des Gashebel fährt das Fahrzeug vorwärts. */
#define STEERING_REMOTE_MIN_RIGHT               STEERING_REMOTE_MID + STEERING_BUFFER_TOLERANCE   /**< Ab diesem Signal des Gashebel fährt das Fahrzeug rückwärts. */
#define STEERING_REMOTE_MIN_MAX_DIF_LEFT        (STEERING_REMOTE_MIN_LEFT-STEERING_REMOTE_MAX_LEFT)
#define STEERING_REMOTE_MIN_MAX_DIF_RIGHT       (STEERING_REMOTE_MAX_RIGHT-STEERING_REMOTE_MIN_RIGHT)

#define ACTIVE_COUNTER_START            5



/*-----------------------------------------STEERING------------------------------------------------------*/
    /**
     * @brief Diese Funktion inialisiert den In- und Output des Lenkungssignals.
     * 
     * Input: 
     * GPIO Pin PB4, Timer 3 Kanal 1 und Interrupt TIM3_IRQn wird konfiguriert.
     * 
     * Output:
     * GPIO Pin PB15, Timer 12 Kanal 2 wird konfiguriert.
     */
    void initSteering(void);
	void TIM3_IRQHandler(void);
void dis_irq_handler(void);
void ultr_irq_handler(void);
    /**
     * @brief Diese Funktion ermöglicht das Ansteuern der Lenkung.
     * 
     * @param[in] direction Lenkrichtung 
     * @param[in] pValue Prozentsatz des einzustellenden Lenkwertes (0 - 100)
     * 
     * Die Ausrichtung erfolgt über die Angabe der Lenkrichtung. 
     * 
     * Der übergebene Wert wird vor dem Setzen überprüft, dass der minimale 
     * und maximale Wert nicht überschritten wird. 
     * Ist der übergebene Wert geringer als der Minimale, so wird der 
     * minimalste Wert gesetzt. Vice Versa.
     */
    void setStreeringFromExtern(uint16_t direction, uint8_t pValue);
	void SteeringTask(void);
	
/*----------------------------------------------POWERTRAIN----------------------------------*/
    /**
     * @brief Diese Funktion inialisiert den In- und Output des Gassignals.
     * 
     * Input: 
     * GPIO Pin PB3, Timer 2 Kanal 2 und Interrupt TIM2_IRQn wird konfiguriert.
     * 
     * Output:
     * GPIO Pin PB14, Timer 12 Kanal 1 wird konfiguriert.
     */
    void initPWRTrain(void);

	void TIM2_IRQHandler(void);

    /**
     * @brief Diese Funktion ermöglicht das Ansteuern des Antriebs.
     * 
     * @param[in] direction Bewegungsrichtung 
     * @param[in] pValue Prozentsatz des einzustellenden Gaswertes (0 - 100)
     * 
     * Der übergebene Wert wird vor dem Setzen überprüft, dass der minimale 
     * und maximale Wert nicht überschritten wird. 
     * Ist der übergebene Wert geringer als der Minimale, so wird der 
     * minimalste Wert gesetzt. Vice Versa.
     */
    void setPWRTrainFromExtern(uint16_t direction, uint8_t pValue);
	void PWRTrainTask(void);

    void initDistanceSensor(void);
	void EXTI15_10_IRQHandler(void);



/*-----------------------------------------------LIGHTING---------------------------------------*/
    /**
     * @brief   Initialisiert die GPIO Pins, welche für die Beleuchtung
     *          verwendet werden.
     * 
     * Bremse:                  C08
     * Blinker links:           C09
     * Blinker rechts:          C11
     * Fernsteuerungs Leuchte:  C13
     */
    void initLights(void);

    /**
     * @brief Steuert die Bremsleuchten an. 
     * @param LIGHT_ON || LIGHT_OFF
     */
    void setStopLights(uint8_t state);

    /**
     * @brief   Steuert die Blinker an und prüft den Timer, ob dieser ausgeführt 
     *          werden muss oder gestoppt werden kann.
     *         
     * EditierenFür das Setzen beider Blinker müssen beide Blinker 
     * nacheinander gesetzt werden. 
     * 
     * @param BLINKER_LIGHTS_INACTIVE || BLINKER_LIGHTS_LEFT_ACTIVE || BLINKER_LIGHTS_RIGHT_ACTIVE
     */
    void setBlinkerLights(uint8_t state);

    /**
     * @brief   Steuert die blaue Signalleuchte an, welche den Zustand der Funk-
     *          Fernbedienung angibt und prüft den Timer, ob dieser ausgeführt 
     *          werden muss oder gestoppt werden kann.
     * 
     * @param LIGHT_ON || LIGHT_OFF
     */
    void setRemoteLight(uint8_t state);




/*----------------------------------------GENERAL--------------------------------------------*/

    typedef struct {
        volatile uint8_t x; /**< Beschleunigung in Richtung der X-Achse. */
        volatile uint8_t y; /**< Beschleunigung in Richtung der Y-Achse. */
        volatile uint8_t z; /**< Beschleunigung in Richtung der Z-Achse. */
    } ACCELEROMETER_T;

    /**
     * @brief   Dieses Struct beinhaltet die Geschwindigkeiten der Räder in m/s.
     */
    typedef struct {
        volatile uint64_t front_left;                     /**< Geschwindigkeit des vorderen linken Rads. */
        volatile uint16_t front_left_rotation;          /**< Anzahl der Umdrehungen des vorderen linken Rads. */
        volatile uint64_t front_right;                    /**< Geschwindigkeit des vorderen rechten Rads. */
        volatile uint16_t front_right_rotation;         /**< Anzahl der Umdrehungen des vorderen rechten Rads. */
        volatile uint64_t back_left;                      /**< Geschwindigkeit des hinteren linken Rads. */
        volatile uint16_t back_left_rotation;           /**< Anzahl der Umdrehungen des hinteren linken Rads. */
        volatile uint64_t back_right;                     /**< Geschwindigkeit des vorderen rechten Rads. */
        volatile uint16_t back_right_rotation;          /**< Anzahl der Umdrehungen des hinteren rechten Rads. */
    } WHEELSPEED_T;

    /**
     * @brief Dieses Struct beinhaltet den Zustand der Leuchten.
     * 
     * Es beinhaltet den Status der Bremsleuchte, des rechten und linken 
     * Blinkers sowie der Leuchte für das Anzeigen der aktiven bzw. inaktiven
     * Funk-Fernbedienung.
     */
    typedef struct {
        volatile uint8_t stop : 1;              /**< Bremsleuchte */
        volatile uint8_t blinker_left : 1;      /**< linker Blinker */
        volatile uint8_t blinker_right : 1;     /**< rechter Blinker */
        volatile uint8_t remote : 1;            /**< blaue Leuchte für den Zustand der Funk-Fernbedienung */
    } LIGHTING_T;

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
        volatile uint8_t steering_direction : 2;        /**< Lenkrichtung */
        volatile uint8_t driving_direction : 2;         /**< Fahrrichtung */
        volatile uint8_t remote : 2;                    /**< Zustand der Fernsteuerung */
        ACCELEROMETER_T accelerometer;                  /**< Beschleunigungswerte */
        WHEELSPEED_T wheelspeed;                        /**< Geschwindigkeiten */
        LIGHTING_T lights;                              /**< Beleuchtung */
        volatile uint16_t front_distance;                 /**< Gemessen Entfernung */
        //volatile uint8_t *picture;                      /**< Kamerabild */
        volatile uint8_t distance_sensor : 1;           /**< Distanzsensor */
		volatile uint8_t modus;							/**< Fahrtmodus *> */
				volatile uint16_t test;
    } RC_STATE_T;


/*-------------------------------------WHEELSENSOR----------------------------------*/

    /**
     * @brief   Dieses Struct wird für das Zwischenspeichern von Zeitstempeln 
     *          für das Errechnen der Rad-Geschwindigkeiten verwendet.
     */
    typedef struct {
        volatile uint64_t timestamp;        /**< Aktueller Zeitstempel */
        volatile uint64_t timestamp_old;    /**< Letzter Zeitstempel */
    } WS_T;
/*--------------------------------------------------------------------------------*/	


/* main.c */
void _start(void);
void main(void);
void drivemode_irq_handler(void);
void vrw_irq_handler(void);
void vlw_irq_handler(void);
void hrw_irq_handler(void);
void hlw_irq_handler(void);
void pwr_irq_handler(void);
void str_irq_handler(void);



/*wheelsensor*/
void initWheelSensors(void);
void EXTI1_IRQHandler(void); 
void EXTI2_IRQHandler(void); 
void EXTI3_IRQHandler(void); 
void EXTI4_IRQHandler(void); 
void EXTI9_5_IRQHandler(void);
int64_t difference_front_wheels(void);
int64_t difference_back_wheels(void);



/*ultrasonic*/
void initUltrasonic(void);
void TIM1_CC_IRQHandler(void);

/* buildid.c -- created at build time */
extern const char __buildid[];

/* stm32f4x7_eth_bsp.c */
void ETH_MACDMA_Config(void);

#endif
