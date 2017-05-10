/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
#include <stdio.h>
#include <string.h>
#include "ch.h"
#include "hal.h"
#include "test.h"
#include "chprintf.h"
#include "shell.h"
#include "LSM6DS3_ACC_GYRO_driver.h"
#include "rtc.h"
//#include "chvt.h"

#define UNUSED(x) (void)(x)
#define BOARD_LED LINE_SAI_SD

struct time_data_t{
  float time;
  int count;
};


/* GLOBAL VARS */
//static virtual_timer_t count_timer;
int count;
float threshold;
struct time_data_t data[1000]; 


/* Thread that blinks North LED as an "alive" indicator */
static THD_WORKING_AREA(waCounterThread,128);
static THD_FUNCTION(counterThread,arg) {
  UNUSED(arg);
  chRegSetThreadName("blinker");
  while (TRUE) {
    palSetLine(BOARD_LED);   
    chThdSleepMilliseconds(500);
    chThdSleepMilliseconds(500);
  }
}
/*******************************
 * ACCELEROMETER FUNCTIONS
 *******************************/
static uint8_t accel_write(u8_t reg, u8_t *data){
	u16_t size = 8;
	return LSM6DS3_ACC_GYRO_WriteReg(NULL, reg, data, size);
}

static uint8_t accel_read(u8_t reg, int *output, u16_t size){
	return LSM6DS3_ACC_GYRO_ReadReg(NULL, reg, data, size);
}

/* Callback for timer, responsible for writing to RAM */
static void proccess_accel(void *arg){
	// get timestampt
	// write data
	// print data
  //  chprintf((BaseSequentialStream*)&SD1, "accel\n\r");
    palToggleLine(BOARD_LED);   
	//reset timer for 60 secs
	//chVTSet(&count_timer, S2ST(60), proccess_accel, NULL);
}

/* 
 * Input		: 	void
 * Purpose	:		Enables accelerometer xyz axes
 * */
static void accel_enable_axes(void){
		uint8_t CTRL9_XL = 0x18; //@TODO add as global in header files
		uint8_t CTRL1_XL = 0x10; 
		uint8_t INT1_CTRL = 0x0D;
		accel_write(CTRL9_XL, 0x38); //Enable axes
		accel_write(CTRL1_XL, 0x60); //Set mode (0x60 = high-performance)
		accel_write(INT1_CTRL, 0x02); //Don't think we need?
}

/* 
 * Input		: 	void
 * Purpose	:		Disables accelerometer xyz axes
 * */
static void accel_disable_axes(void){
		

}


/*
 * Input		: data_out - float[] array size three for x,y,z axes
 * Purpose	: read
 * Prereq		: accel_enable_axes() should be called
 * */
static void accel_read_data(float *data_out){
	//see page 37 app notes
}

/*******************************
 * SHELL FUNCTIONS
 *******************************/


static void cmd_myecho(BaseSequentialStream *chp, int argc, char *argv[]) {
  int32_t i;
  (void)argv;
  for (i=0;i<argc;i++) {
    chprintf(chp, "%s\n\r", argv[i]);
  }
}

static void cmd_accel_wai(BaseSequentialStream *chp, int argc, char *argv[]) {
  uint8_t wai;
  (void)argv;
  (void)argc;

  LSM6DS3_ACC_GYRO_R_WHO_AM_I(NULL, &wai);
  chprintf(chp,"lsm6ds3 Who Am I data = 0x%02x\n\r",wai);
}


static void cmd_display_counts(BaseSequentialStream *chp, int argc, char *argv[]){
	(void)argv;
	(void)argc; //What is the purpose of these? @TODO ask 
	uint16_t data[3];
	LSM6DS3_ACC_Get_Acceleration(NULL, &data, 0);
	chprintf(chp, "OUTX: %d\r\n", data[1]);
}

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

static const ShellCommand commands[] = {
  {"myecho", cmd_myecho},
  {"accel_wai",cmd_accel_wai},
	{"display_counts", cmd_display_counts},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SD5,
  commands
};

/*
 * Application entry point.
 */

int main(void) {
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  static thread_t *shelltp = NULL;

  halInit();
  chSysInit();


  /*
   * Activates the serial driver 5 using the driver default configuration.
   * PC12(TX) and PD2(RX). The default baud rate is 38400.
   */
  sdStart(&SD5, NULL);
  chprintf((BaseSequentialStream*)&SD5, "\n\rUp and Running\n\r");

  /* Initialize the command shell */ 
  shellInit();
  chThdCreateStatic(waCounterThread, sizeof(waCounterThread), NORMALPRIO+1, counterThread, NULL);

  /* Initialize the Accelerometer */
  LSM6DS3_ACC_GYRO_Init();

	/*Initialize minute timer */
//	chVTObjectInit(&count_timer);
	//chVTSet(&count_timer, S2ST(60), proccess_accel, NULL);

	
  while (TRUE) {
    if (!shelltp) {
      shelltp = shellCreate(&shell_cfg1, SHELL_WA_SIZE, NORMALPRIO);
    }
    else if (chThdTerminatedX(shelltp)) {
      chThdRelease(shelltp);    /* Recovers memory of the previous shell.   */
      shelltp = NULL;           /* Triggers spawning of a new shell.        */
    }
    chThdSleepMilliseconds(1000);
  }
}
