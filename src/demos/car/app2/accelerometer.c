/** 
 * @file        accelerometer.c
 * @author      Marcel Kneib
 * @date        01.08.2013
 * @brief       Dieser Datei beinhaltet die Implementierung, um die 
 *              Beschleunigungswerte des LIS302D zu ermitteln.
 * 
 * Durch die 100 Interrupts/Sekunde des LIS302D, wird eine interne Liste
 * aktualisiert, welche eine bestimmte Anzahl von Werten des Sensors enthält.
 * Anschließend wird der Mittelwert dieser Liste berechnet und im global 
 * verfügbaren Struct aktualisiert. 
 * 
 */

#include "app.h"
#include <stdio.h>

#define ACCELEROMETER_VAL_COUNT 10      /**< Die Anzahl der Werte zur Ermittlung der Beschleunigung. ACHTUNG: Bei Änderung acc_fifi.index anpassen. */

/**
 * @brief Beinhaltet die zur Ermittlung gespeicherten Zwischenwerte.
 */
static struct {
    volatile ACCELEROMETER_T buffer[ACCELEROMETER_VAL_COUNT];
    volatile uint8_t index : 4; /**< Index des ältesten Wertes. */
} acc_fifo;

uint8_t accelerBuffer[6]; /**< Buffer für das auslesen des Sensors. */
RC_STATE_T rc_state; /**< Das externe Struct, welches die Systemwerte enthält. */

static void init_acc_fifo(void);
static void read_value_acc_fifo(void);
static void update_avg_acc_fifo(void);
static void info(void);
void init_exti(void);
void close_interrupt(void);

void initAccelerometer() {
    LIS302DL_InitTypeDef LIS302DL_InitStruct;
    LIS302DL_InitStruct.Power_Mode = LIS302DL_LOWPOWERMODE_ACTIVE;
    LIS302DL_InitStruct.Output_DataRate = LIS302DL_DATARATE_100;
    LIS302DL_InitStruct.Axes_Enable = LIS302DL_XYZ_ENABLE;
    LIS302DL_InitStruct.Full_Scale = LIS302DL_FULLSCALE_2_3;
    LIS302DL_InitStruct.Self_Test = LIS302DL_SELFTEST_NORMAL;
    LIS302DL_Init(&LIS302DL_InitStruct);
    // Anschalten dauert ca. 30ms
    sys_sleep(100000000);


    //Fifo initialisieren
   init_acc_fifo();

	close_interrupt(); 
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource0);
	init_exti();


}

void close_interrupt() {

	 //Interrupt anschalten
    uint8_t ctrl = 0x04;
    LIS302DL_Write(&ctrl, LIS302DL_CTRL_REG3_ADDR, 1); 
 		
}

void init_exti() {
	EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
}

/**
 * @brief       Interrupt, welcher durch den LIS302D ausgelöst wird, wenn ein
 *              neuer Wert vorliegt.
 * 
 * Hier wird der aktuelle Wert des Sensors ausgelesen, die interne Liste 
 * aktualisiert und der daraus resultierende Mittelswert ermittelt und im 
 * globalen Struct aktualisiert.
 */
void EXTI0_IRQHandler(void) {
 printf(".\n");
    if (EXTI_GetITStatus(EXTI_Line0) != RESET) {

		read_value_acc_fifo();
		
        update_avg_acc_fifo();

		printf("X: %i\n", rc_state.accelerometer.x);	
		printf("Y: %i\n",rc_state.accelerometer.y);	
		printf("Z: %i\n",rc_state.accelerometer.z);	
        EXTI_ClearITPendingBit(EXTI_Line0);
        /* Clear the EXTI line 0 pending bit */
    }
}

void AccelerometerTask() {
	initAccelerometer();
	info();
	update_avg_acc_fifo();
    printf("TASK: Accelerometer:X:%3d\tY:%3d\tZ:%3d\n\r", rc_state.accelerometer.x, rc_state.accelerometer.y, rc_state.accelerometer.z);
}



/**
 * Initialisiert die interne Liste mit Werten des Sensors.
 */
 static void init_acc_fifo() {
    uint8_t tmp = 0;

    for (acc_fifo.index = 0; acc_fifo.index < ACCELEROMETER_VAL_COUNT; acc_fifo.index++) {
        do {
		
            LIS302DL_Read(&tmp, LIS302DL_STATUS_REG_ADDR, 1);
        } while (!(tmp & 0x08));
       // LIS302DL_Read(accelerBuffer, LIS302DL_OUT_X_ADDR, 6);
	
        if (accelerBuffer[0] >= 128) accelerBuffer[0] = 255 - accelerBuffer[0];
        if (accelerBuffer[2] >= 128) accelerBuffer[2] = 255 - accelerBuffer[2];
        if (accelerBuffer[4] >= 128) accelerBuffer[4] = 255 - accelerBuffer[4];

        acc_fifo.buffer[acc_fifo.index].x = accelerBuffer[0];
        acc_fifo.buffer[acc_fifo.index].y = accelerBuffer[2];
        acc_fifo.buffer[acc_fifo.index].z = accelerBuffer[4];
    } 
    acc_fifo.index = 0;
    update_avg_acc_fifo();
}

/**
 * Liest den aktuellen Wert des Sensors und speichert diesen in der internen 
 * Liste.
 */
static void read_value_acc_fifo() {
    uint8_t tmp = 0;		
    do {	

     LIS302DL_Read(&tmp, LIS302DL_STATUS_REG_ADDR, 1);
    } while (!(tmp & 0x08));
  //  LIS302DL_Read(accelerBuffer, LIS302DL_OUT_X_ADDR, 6);

    // Der Sensor sendet von 0 bis 180° einen Wert zwischen 0 und 128.
    // Zwischen 180° und 360° sendet er einen Wert zwischen 128 und 255.
    // Da 255 jedoch ca 5g widerspiegeln würde, muss der Wert hier angepasst
    // werden. Dadurch geht jedoch die Information verloren, in welche
    // Richtung sich das Fahrzeug bewegt. 

    // 128 <-----> 0|255 <------> 128
    if (accelerBuffer[0] >= 128) accelerBuffer[0] = 255 - accelerBuffer[0];
    if (accelerBuffer[2] >= 128) accelerBuffer[2] = 255 - accelerBuffer[2];
    if (accelerBuffer[4] >= 128) accelerBuffer[4] = 255 - accelerBuffer[4];

    acc_fifo.buffer[acc_fifo.index].x = accelerBuffer[0];
    acc_fifo.buffer[acc_fifo.index].y = accelerBuffer[2];
    acc_fifo.buffer[acc_fifo.index].z = accelerBuffer[4];
    if (acc_fifo.index >= ACCELEROMETER_VAL_COUNT - 1) {
        acc_fifo.index = 0;
    } else {
        acc_fifo.index++;
    }

}

/**
 * Ermittelt den Mittelwert aus den gespeicherten Werten der internen Liste.
 */


static void update_avg_acc_fifo() {
    volatile int32_t sumx = 0, sumy = 0, sumz = 0;

    // Falls zu viele Ausreiser auftreten, kann dieses Codeteil genutzt werden.
    // Es bestimmt einen Mittelwert und akzeptiert nur Werte die kleiner als
    // 2 * dieser Wert sind, für die Berechnung des neuen Mittelwerts.
    //------------------------------------------------------------------------
    //    sumx = 0;
    //    sumy = 0;
    //    sumz = 0;
    //    tmpx = 0;
    //    tmpy = 0;
    //    tmpz = 0;
    //    for (int8_t i = 0; i < ACCELEROMETER_VAL_COUNT; i++) {
    //        tmpx += acc_fifo.buffer[i].x;
    //        tmpy += acc_fifo.buffer[i].y;
    //        tmpz += acc_fifo.buffer[i].z;
    //    }
    //    tmpx /= ACCELEROMETER_VAL_COUNT;
    //    tmpy /= ACCELEROMETER_VAL_COUNT;
    //    tmpz /= ACCELEROMETER_VAL_COUNT;
    //
    //    for (int8_t i = 0; i < ACCELEROMETER_VAL_COUNT; i++) {
    //        if (acc_fifo.buffer[i].x < 2 * tmpx) sumx += acc_fifo.buffer[i].x;
    //        if (acc_fifo.buffer[i].y < 2 * tmpy) sumy += acc_fifo.buffer[i].y;
    //        if (acc_fifo.buffer[i].z < 2 * tmpz) sumz += acc_fifo.buffer[i].z;
    //    }
    // Wenn dieser Teil genutzt wird, den darunterliegenden auskommentieren.
    //------------------------------------------------------------------------

    for (int8_t i = 0; i < ACCELEROMETER_VAL_COUNT; i++) {
        sumx += acc_fifo.buffer[i].x;
        sumy += acc_fifo.buffer[i].y;
        sumz += acc_fifo.buffer[i].z;
    }
    //------------------------------------------------------------------------

    rc_state.accelerometer.x = (sumx / ACCELEROMETER_VAL_COUNT);// * MG_PER_DIGIT_2G; Berechnung nun auf Pi
    rc_state.accelerometer.y = (sumy / ACCELEROMETER_VAL_COUNT);// * MG_PER_DIGIT_2G;
    rc_state.accelerometer.z = (sumz / ACCELEROMETER_VAL_COUNT);// * MG_PER_DIGIT_2G;
}

/**
 * Dient lediglich zum Testen. Gibt alle Werte der Liste aus.
 */
static void info() {
    volatile int32_t sumx = 0, sumy = 0, sumz = 0;
    for (uint8_t i = 0; i < ACCELEROMETER_VAL_COUNT; i++) {
        printf("[%x]\tX:%d\tY:%d\tZ:%d\t\n\r", i, (int)acc_fifo.buffer[i].x, (int)acc_fifo.buffer[i].y, (int)acc_fifo.buffer[i].z);
        sumx += acc_fifo.buffer[i].x;
        sumy += acc_fifo.buffer[i].y;
        sumz += acc_fifo.buffer[i].z;
    }
    printf("S:\tX:%d\tY:%d\tZ:%d\t\n\r", (int)sumx, (int)sumy, (int)sumz);
    update_avg_acc_fifo();
    printf("S:\tX:%d\tY:%d\tZ:%d\t\n\r", (int)rc_state.accelerometer.x, (int)rc_state.accelerometer.y, (int)rc_state.accelerometer.z);
}

