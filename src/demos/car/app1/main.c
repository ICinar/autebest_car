/** @mainpage Anforderungsanalyse, Entwurf und Entwicklung einer Plattform für ein autonomes Modellfahrzeug 
 *  
 *  @section sec1 Einleitung 
 *  Dieses Projekt entstand im Rahmen des Bachelorstudiengangs "Angewandte Informatik". Es dient dazu, ein 
 *  Modellfahrzeug zu steuern sowie angebrachte Sensoren auszuwerten. Weiterhin bietet sie einen zentralen
 *  Einstiegspunkt, an der eine autonome Steuerung implementiert werden kann. 
 *   
 *  @section sec2 Autonome Steuerung - Task
 *  Für die Implementierung der autonomen Steuerung steht ein Task zur Verfügung. Dieser wird aktiviert,
 *  sobald die Funk-Fernbedienung des Fahrzeugs deaktiviert wird. Dieser Task wird beendet, sobald die
 *  Fernsteuerung wieder aktiviert wird. 
 *  Die Implementierung des Tasks befindet sich in der "autonomous.c" und der "autonomous.h". 
 *  Die Konfiguration der Priorität sowie der Periodendauer erfolgt über die Headerdatei "general.h". 
 *  
 *  Für die Wahrnehmung der Umgebung stehen umfassende Sensoren zur Verfügung, deren Werte mittels 
 *  einer globalen Datenstruktur ausgewertet werden können. 
 *  
 *  @subsection sec2_1 Steuerung
 *  Für die autonome Steuerung des Antriebs steht die Funktion "setPWRTrainFromExtern" zur Verfügung. 
 *  Mittels dieser kann die Fahrrichtung ausgewählt werden sowie die Gasstellung auswählen. 
 *  
 *  \code{.cpp}
 *      setPWRTrainFromExtern(PWRTRAIN_DIR_FORWARDS, 50);
 *  \endcode
 *  
 *  Mittels dieser Anweisung wird die Fahrtrichtung auf vorwärts gesetzt und die Geschwindigkeit
 *  auf 50% des maximalen Wertes eingestellt. 
 * 
 *  Ebenso kann das Fahrzeug über diese Anweisung zum Bremsen sowie zu loslassen der Bremse 
 *  veranlasst werden. Dies ist notwendig, da das Bremsen dadurch ausgelöst wird, dass der Antrieb 
 *  während der Vorwärtsfahrt ein Signal zur Rückwärtsfahrt übermittelt bekommt. Für das anschließende
 *  Rückwärtsfahren muss der Antrieb auf den Neutralen wert eingestellt werden. 
 *  \code{.cpp}
 *      setPWRTrainFromExtern(PWRTRAIN_DIR_BRAKING, 0);
 *      //...
 *      // Warten bis das Fahrzeug gestopppt ist. 
 *      //...
 *      setPWRTrainFromExtern(PWRTRAIN_DIR_STOPPED, 0);
 *      // Warten bis das Fahrzeug dieses Signal erfasst hat. ~34ms
 *  \endcode
 * \n
 *  * Direction            |Lenkstellung  
 * -----------------------|--------------
 * PWRTRAIN_DIR_FORWARDS  | 0-100 
 * PWRTRAIN_DIR_BACKWARDS | 0-100  
 * PWRTRAIN_DIR_BRAKING   | nicht beachtet
 * PWRTRAIN_DIR_STOPPED   | nicht beachtet
 * 
 *  Für das Wechseln der Richtung ist das Bremsen des Fahrzeugs notwendig, sofern das Fahrzeug von der
 *  Fortwärtsfahrt in die Rückwärtsfahrt versetzt werden soll. 
 * 
 * 
 *  Für die Ansteuerung der Lenkung wird die Funktion "setStreeringFromExtern". Mittels dieser wird 
 *  das PWM-Signal des Servos angesteuert.
 * 
 *  \code{.cpp}
 *      setStreeringFromExtern(STEERING_DIR_RIGHT, 50);
 *  \endcode  
 * 
 *  Mittels dieser Anweisung wird die Lenkrichtung auf rechts gesetzt und der Lenkeinschlag auf 50% des 
 *  maximalen Lenkwinkels eingestellt.

 * Direction            |Lenkstellung  
 * ---------------------|--------------
 * STEERING_DIR_RIGHT   | 0-100 
 * STEERING_DIR_LEFT    | 0-100  
 * STEERING_DIR_NEUTRAL | nicht beachtet
 * 
 * 
 *  @subsection sec2_2 Sensoren  
 *  Auf die Messwerte der am Fahrzeug angebrachten Sensoren kann durch die globale Datenstruktur zugegriffen
*  werden. Diese Datenstruktur wird mittels "extern RC_STATE_T rc_state;" eingebunden und kann
*  in der Umgebung verwendet werden. 
* 
*  Die Beschleunigungswerte befinden sich in einer weiteren Datenstruktur, welche die Werte der X-, Y- und
*  Z-Achse beinhaltet. rc_state.accelerometer.x, rc_state.accelerometer.y, rc_state.accelerometer.z = 0x0;
*  Diese Werte sind in der Einheit mg.
* 
*  Die Erkennung des Distanzsensors an der rechten Seite des Fahrzeugs, erfolgt durch die Variable 
*  "rc_state.distance_sensor". Diese kann den Wert "DISTANCE_NO_DETECTION" oder "DISTANCE_DETECTION" enthalten.
*   
*  Für die Auswertung des Ultraschallsensors an der Front des Fahrzeugs steht die Variable "rc_state.front_distance"
*  zur Verfügung. Diese enthält die aktuelle Entfernung in cm.
* 
*  Für das Ermitteln des Zustandes der Funk-Fernbedienung steht die Variable "rc_state.remote" zur Verfügung, welche
*  mit dem Wert von "REMOTE_INAKTIVE" oder "REMOTE_AKTIVE" versehen sein kann.
* 
*  Die Geschwindigkeiten und Umdrehungen der einzelnen Räder befinden sich in einer weiteren Datenstruktur. Diese gibt
*  die Geschwindigkeiten in m/s an. Diese Werte befinden sich hinter der Datenstruktur "rc_state.wheelspeed".
* 
*  Für das Auswerten des Kamerabildes steht ein Pointer zur Verfügung, welcher auf eine Adresse zeigt, an der die
*  Kamera die Bilder mittels DMA ablegt. 
* 
*  Für das Setzen der Signalleuchten stehen Funktionen zur Verfügung. 
*  
* Funktion          |Beleuchtung    |Werte
* ------------------|---------------|-------------
* setStopLights     | Bremsleuchten |LIGHT_ON, LIGHT_OFF\n
* setBlinkerLights  | Blinker       |BLINKER_LIGHTS_LEFT_ACTIVE, BLINKER_LIGHTS_RIGHT_ACTIVE,BLINKER_LIGHTS_INACTIVE

*/


/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
/* Application include. */
#include "app.h"


extern RC_STATE_T rc_state; /**< Globale Datensturktur*/

/**
 * @brief   Initialisiert das System und startet anschließend den Scheduler.
 * 
 * Initialisierung:      
 *  - Globale Datensturktur 
 *  - Funktionen
 *  - Tasks
 */

/* Stacks */
struct {
	char s[1024];
} __stack_main __aligned(16);
struct {
	char s[1024];
} __stack_isr __aligned(16);
struct {
	char s[1024];
} __stack2 __aligned(16);
struct {
	char s[1024];
} __stack3 __aligned(16);
struct {
	char s[1024];
} __stack_isr1 __aligned(16);
struct {
	char s[1024];
} __stack_isr2 __aligned(16);
struct {
	char s[1024];
} __stack_isr3 __aligned(16);
struct {
	char s[1024];
} __stack_isr4 __aligned(16);
struct {
	char s[1024];
} __stack_isr5 __aligned(16);
struct {
	char s[1024];
} __stack_isr6 __aligned(16);
struct {
	char s[1024];
} __stack_isr7 __aligned(16);
struct {
	char s[1024];
} __stack_isr8 __aligned(16);
struct {
	char s[1024];
} __stack_isr9 __aligned(16);
struct {
	char s[1024];
} __stack_isr10 __aligned(16);
/* static storage */
/* static storage */




/* IRQ call back */
void dis_irq_handler(void)
{
	printf("Distanz\n");	
	EXTI15_10_IRQHandler();	 	
	sys_task_terminate();
	sys_abort();
}

void ultr_irq_handler(void)
{
	printf("ENTFERNUNG\n");	
 	TIM1_CC_IRQHandler();
	sys_task_terminate();
	sys_abort();
}

void vrw_irq_handler(void)
{
 //	(*(volatile uint32_t *)0x40013c14) = 0x0008;	
	EXTI3_IRQHandler();
	sys_task_terminate();
	sys_abort();
}
void vlw_irq_handler(void)
{
 	//(*(volatile uint32_t *)0x40013c14) = 0x0001;	
	EXTI1_IRQHandler();

	sys_task_terminate();
	sys_abort();
}

void hrw_irq_handler(void)
{
 	//(*(volatile uint32_t *)0x40013c14) = 0x0010;	
	EXTI9_5_IRQHandler();	
	sys_task_terminate();
	sys_abort();
}

void hlw_irq_handler(void)
{
 	//(*(volatile uint32_t *)0x40013c14) = 0x0004;	
	EXTI2_IRQHandler();
	sys_task_terminate();
	sys_abort();
}


void pwr_irq_handler(void)
{		
	TIM2_IRQHandler();
	sys_task_terminate();
	sys_abort();
}

void str_irq_handler(void)
{			
	TIM3_IRQHandler();
	sys_task_terminate();
	sys_abort();
}


/* startup code + main */
void _start(void)
{
	unsigned int err;
	/* NOTE: start next task via chaining, otherwise on ARM Cortex-M3/M4
	 * the shared stack gets corrupted by the new task's registers
	 */
	err = sys_task_chain(CFG_TASK_main);
	assert(err == E_OK);
	(void)err;

}

void main(void) {
	//prvSetupHardware();
	/*
	 ****************************************************************************
	 *   GLOBALE DATENSTRUKTUR INITIALISIERUNG 
	 ****************************************************************************
	 */


	rc_state.accelerometer.x = 0x0;
	rc_state.accelerometer.y = 0x0;
	rc_state.accelerometer.z = 0x0;
	rc_state.distance_sensor = DISTANCE_NO_DETECTION;
	rc_state.driving_direction = PWRTRAIN_DIR_STOPPED;
	rc_state.front_distance = 0x0;
	rc_state.modus = 0;
	rc_state.lights.blinker_left = LIGHT_OFF;
	rc_state.lights.blinker_right = LIGHT_OFF;
	rc_state.lights.remote = LIGHT_OFF;
	rc_state.lights.stop = LIGHT_OFF;	
	rc_state.remote = REMOTE_INAKTIVE;
	rc_state.steering_direction = STEERING_DIR_NEUTRAL;
	rc_state.wheelspeed.front_left = 0;
	rc_state.wheelspeed.back_left = 0;
	rc_state.wheelspeed.front_right = 0;
	rc_state.wheelspeed.back_right = 0;
	/*
	 ****************************************************************************
	 *   FUNKTION INITIALISIERUNG 
	 ****************************************************************************
	*/
	initUltrasonic(); /* Initialisierung Ultraschallsensor */
	initSteering(); /* Initialisierung Steuerung */
	initPWRTrain(); /* Initialisierung Antrieb */
	initLights(); /* Initialisierung Signalleuchten */
	initWheelSensors(); /* Initialisierung Radsensoren */
	initDistanceSensor(); /* Initialisierung Distanzsensor */
	


	//Startet Scheduling Tabelle
	sys_schedtab_start_rel(CFG_SCHEDTAB_car,3);

	printf("nach Task Partition 1 init\n");
	sys_task_terminate();
	sys_abort();
}

