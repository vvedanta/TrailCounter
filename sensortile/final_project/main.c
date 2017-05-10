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
#include <stdlib.h>
#include <string.h>
#include "ch.h"
#include "hal.h"
#include "test.h"
#include "chprintf.h"
#include "shell.h"
#include "LSM303AGR_ACC_driver.h"
#include "rtc.h"
#include "time.h"
#include "string.h"
#include "counter_config.h"
#include "emu_uart.h"



#define UNUSED(x) (void)(x)
#define BOARD_LED LINE_SAI_SD

/* GLOBAL VARS */
//password for sim: VinAndMagsKillingTheGame
//phone num: 8122873157
int people_count = 0; //Current total of people/min
int people_counts_in_a_row = 0; // Checks if our calibration is off
int zero_counts_in_a_row = 0;
int calibrated_axes[3]  = {0, 0, 1000};  //Default accelerometer calibration initialization. 
				 //Z-axis is 1 G to offset gravity
int start_tcp = 0;
int i; //iterator util
struct time_data_t timestamp_data[MAX_TIMESTAMPS]; //Holds timestamps & counts.
int num_timestamps = 0; //Counts number of readings. Should always < MAX_TIMESTAMPS
RTCDateTime start_time; //@TODO
EUARTHandle EUARTH;

/*******************************
 * UTIL FUNCTIONS
 *******************************/
#define MAX_FILLER 11
#define FLOAT_PRECISION 9

static char *long_to_string_with_divisor(char *p,
                                         long num,
                                         unsigned radix,
                                         long divisor) {
  int i;
  char *q;
  long l, ll;

  l = num;
  if (divisor == 0) {
    ll = num;
  } else {
    ll = divisor;
  }

  q = p + MAX_FILLER;
  do {
    i = (int)(l % radix);
    i += '0';
    if (i > '9')
      i += 'A' - '0' - 10;
    *--q = i;
    l /= radix;
  } while ((ll /= radix) != 0);

  i = (int)(p + MAX_FILLER - q);
  do
    *p++ = *q++;
  while (--i);

  return p;
}

static char *ch_ltoa(char *p, long num, unsigned radix) {

  return long_to_string_with_divisor(p, num, radix, 0);
}

/*******************************
 * CELL FUNCTIONS
 *******************************/
/*static void write_to_Modem(char * command){
	//todo allow to read as well.
	chprintf((BaseSequentialStream*)&SD5, "%s", command);
}

static void read_from_modem(char * cmd){
	//@TODO fix so it reads instead.
	chprintf((BaseSequentialStream*)&SD5, "%s", cmd);
}

static void send_SMS(char * message) {
	//Check for valid message length.
	//Check that phone number is set. 
	//Use AT command
	write_to_Modem(message);
}
*/

/*******************************
 * THREADS
 *******************************/

/* Thread that blinks North LED as an "alive" indicator */
static THD_WORKING_AREA(waCounterThread,128);
static THD_FUNCTION(counterThread,arg) {
  UNUSED(arg);
  chRegSetThreadName("blinker");
  while (TRUE) {
    palSetLine(BOARD_LED);   
    chThdSleepMilliseconds(500);
		palClearLine(BOARD_LED);
    chThdSleepMilliseconds(500);
  }
}

/* Send Data via TCP Socket */
static THD_WORKING_AREA(waTCPThread, 4096);
static THD_FUNCTION(tcpThread,arg) {
  UNUSED(arg);
  chRegSetThreadName("socket");
  chThdSleepSeconds(5);
  while (TRUE) {
  	if(start_tcp){

  	// 1. system power on/Check that system is on
  	chprintf((BaseSequentialStream*)&SD5, "\n\rSTARTING TCP\n\r");

  	EUARTSendCommand(&EUARTH, "AT", 2);
    chThdSleepSeconds(1);
	//EUARTRecvResponse("OK");
  	// 2. query and wait for GPRS to attach to network
  	EUARTSendCommand(&EUARTH, "AT+CGATT?", 9);
    chThdSleepSeconds(2);
  	//EUARTRecvResponse("+CGATT: 1");
  	// 3. Reset any running IP session
  	EUARTSendCommand(&EUARTH, "AT+CIPSHUT", 10);
    chThdSleepSeconds(2);
	//EUARTRecvResponse("SHUT OK");
  	// 4. Check if IP stack is initialized
  	EUARTSendCommand(&EUARTH, "AT+CIPSTATUS", 12);
    chThdSleepSeconds(2);
	//EUARTRecvResponse("STATE: IP INITIAL");
  	// 5. Set Connection mode to single
  	EUARTSendCommand(&EUARTH, "AT+CIPMUX=0", 11);
    chThdSleepSeconds(1);
	//EUARTRecvResponse("OK");
	// 6. Set APN
	// char* at_command = "AT+CSTT=";
	// char* apn = TPN_APN;
	// strcat(at_command, apn);
  	EUARTSendCommand(&EUARTH, "AT+CSTT=\"wholesale\"", 19); //tpo.pmvne
    chThdSleepSeconds(2);
	//EUARTRecvResponse("OK");
  	// 7. Bring wireless up
  	EUARTSendCommand(&EUARTH, "AT+CIICR", 8);
    chThdSleepSeconds(15);
  	//EUARTRecvResponse("OK");
  	// 8. Get Local IP Address 
  	EUARTSendCommand(&EUARTH, "AT+CIFSR", 8);
    chThdSleepSeconds(1);
	// 9. Connect socket via TCP
  	EUARTSendCommand(&EUARTH, "AT+CIPSTART=\"TCP\",\"silo.cs.indiana.edu\",\"50106\"", 47);
    chThdSleepSeconds(5);
	// EUARTRecvResponse("CONNECT OK")
  	//10. Transfer data
    char sendCo[32];
    strcpy(sendCo,"AT+CIPSEND=");
    int numStored = num_timestamps * 34;
    char charStored[32];
    ch_ltoa(charStored, numStored, 10);
    strcat(sendCo, charStored);
    EUARTSendCommand(&EUARTH, sendCo, strlen(sendCo));
    chThdSleepSeconds(1);
  	int i;
    struct tm timeptr;
    uint32_t ms;
    char tm_str[32];
    char count_str[16];
    int bytes_sent = 0;
  	for (i = 0; i < num_timestamps; i++) {
	    rtcConvertDateTimeToStructTm(&timestamp_data[i].time, &timeptr, &ms);
  		strftime(tm_str, 31, "%c", &timeptr);
	    EUARTSendCommand(&EUARTH, tm_str, strlen(tm_str)); //strlen(tm_str)
  		EUARTSendCommand(&EUARTH, ",", 1);
  		ch_ltoa(count_str, (long)timestamp_data[i].people_count, 10);
  		EUARTSendCommand(&EUARTH, count_str, strlen(count_str));
	    EUARTSendCommand(&EUARTH, "\n", 1);
      bytes_sent += (strlen(tm_str) + strlen(count_str) + 6);
	  }
    for (i = 0; i < (numStored - bytes_sent); i+=2) {
      EUARTSendCommand(&EUARTH, " ");
    }
	// 12. Close TCP connection
	  EUARTSendCommand(&EUARTH, "AT+CIPSHUT", 10);
	//EUARTRecvResponse("SHUT OK");
	// 13. Sleep until the next transfer
	  }
	  //start_tcp = 0;
	  chThdSleepSeconds(TCP_FREQ);
	}
} 

/* Time Stamp */
static THD_WORKING_AREA(waTimeStamp,4096);
static THD_FUNCTION(timeStamp,arg) {
        UNUSED(arg);
        chRegSetThreadName("timestamp");
        while (TRUE) {
                rtcGetTime(&RTCD1, &(timestamp_data[num_timestamps].time)); 
                rtcSetTime(&RTCD1, &(timestamp_data[num_timestamps].time)); //Write the time as a backup if the board fails/resets 
				if((num_timestamps < MAX_TIMESTAMPS - 1)){ //Stops writing when memory full
					if(people_count == 0){ //Checks for low calibration
						zero_counts_in_a_row++;
						people_counts_in_a_row = 0;
					}
					else{
						zero_counts_in_a_row = 0;
						people_counts_in_a_row++;
					}
					if(!(SS_FLAG) || (SS_FLAG && people_count > 0)){						
				        timestamp_data[num_timestamps].people_count = people_count;
				        people_count = 0;
						num_timestamps++;
					}

					if (zero_counts_in_a_row * TIMESTAMP_FREQ > RECALIBRATION_FREQ ||
						people_counts_in_a_row * TIMESTAMP_FREQ > RECALIBRATION_FREQ){
						//Time to try re-calibrating
						LSM303AGR_ACC_Get_Acceleration(NULL, calibrated_axes);
//	  					chprintf((BaseSequentialStream*)&SD5, "\rWarning: Accelerometer Re-calibrated\r\nPrevious data may not be accurate.\r\n");
	  					rtcGetTime(&RTCD1, &(timestamp_data[num_timestamps].time));
	  					if (zero_counts_in_a_row > 0) //@TODO make these macros
	  					{		timestamp_data[num_timestamps].people_count = -1;	}
	  					else{ 	timestamp_data[num_timestamps].people_count = -2; } //Signals re-calibration points
	  					num_timestamps++;
	  					zero_counts_in_a_row = 0;
	  					people_counts_in_a_row =0 ;
					}
				}


				
                chThdSleepSeconds(TIMESTAMP_FREQ);
        }
}
/*
 *   Check Threshold
 *  */

THD_WORKING_AREA(waCheckThreshold, 4096);
THD_FUNCTION(checkThreshold, arg) {

        (void)arg;
        chRegSetThreadName("checkthreshold");
        int data[3];
	int curr_accel;
        while(true) {
                LSM303AGR_ACC_Get_Acceleration(NULL, data);
                curr_accel = abs(data[0] - calibrated_axes[0]) 
														+ abs(data[1]-calibrated_axes[1]) 
														+ abs(data[2]-calibrated_axes[2]); //sum total accel, offset by calibration
				
                if (curr_accel > MOVEMENT_THRESHOLD) {
                        people_count++;
                        //bullshit_checker++;
                } else {
                	//bullshit_checker = 0;
                }

                /*if (bullshit_checker > BULLSHIT_THRESHOLD) {
                	LSM303AGR_ACC_Get_Acceleration(NULL, calibrated_axes);
  					chprintf((BaseSequentialStream*)&SD5, "\rWarning: Accelerometer Re-calibrated\r\nPrevious data may not be accurate.\r\n");
  					rtcGetTime(&RTCD1, &(timestamp_data[num_timestamps].time));
  					timestamp_data[num_timestamps].people_count = -1; //Signal re-calibration points in data
  					num_timestamps++;
  					bullshit_checker = 0;
                }*/
                chThdSleepMilliseconds(ACCEL_MOVEMENT_FREQ);
        }
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

//Makes a call
static void cmd_call_maggie(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  (void)argc;

	char * cmd =  "ATD17654041973;";
	EUARTSendCommand(&EUARTH, cmd, 15);

		chprintf(chp, "Sent call.\n\r");
}

static void cmd_modem_cmd(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  (void)argc;
	
	if(argc != 1){		chprintf(chp, "Bad args.\n\r"); }
	else{
	char * cmd =  argv[0];
	
	EUARTSendCommand(&EUARTH, cmd, strlen(cmd));

		chprintf(chp, "Sent call.\n\r");
	}
}
//Transmits one char over emulated UART
static void cmd_put_char(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  (void)argc;

	//@TODO allow for multi-letter chars, e.g. '/n'
	if(argc == 1){
		char letter = argv[0][0];
		int put_return = EUARTPutChar(&EUARTH, letter);
//    EUARTGetChar(&EUARTH, &ch);
    chprintf(chp, "\n%d\n\r", put_return);
	}
	else{
		chprintf(chp, "Incorrect args.\n\r");
	}
}

//Receives one char over emulated UART, or times out
static void cmd_get_char(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  (void)argc;

	uint8_t code;
	char ch;
	int i;
//	for(i = 0; i < 5; i++){
					code = EUARTGetChar(&EUARTH, &ch);
					if(code == EUART_TIMEOUT){
						chprintf(chp, "EUART Timed out\n\r");
					}
					else if(code == EUART_ERROR){
						chprintf(chp, "EUART Error\n\r");
					}
					chprintf(chp, "\ngot char: %c aka 0x%x\n\r", ch);
//	}
}

static void cmd_test_char(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  (void)argc;
	char letter = '\0';

	if(argc == 1){
		letter = argv[0][0];}
	else	{
		chprintf(chp, "Incorrect args.\n\r");}

	uint8_t code;
	char ch;
	int i;
	for(i = 0; i < 5; i++){
					
					int put_return = EUARTPutChar(&EUARTH, letter);
					code = EUARTGetChar(&EUARTH, &ch);
					if(code == EUART_TIMEOUT){
						chprintf(chp, "EUART Timed out\n\r");
					}
					else if(code == EUART_ERROR){
						chprintf(chp, "EUART Error\n\r");
					}
					chprintf(chp, "\ngot char: %c aka 0x%x, compare to %c\n\r", ch, (int) ch, letter);
	}
}

//Prints whoami value for accel
static void cmd_accel_wai(BaseSequentialStream *chp, int argc, char *argv[]) {
  uint8_t wai;
  (void)argv;
  (void)argc;

  LSM303AGR_ACC_R_WHO_AM_I(NULL, &wai);
  chprintf(chp,"lsm303 Who Am I data = 0x%02x\n\r",wai);
}

//Prints current reading on accel
static void cmd_accel_print(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  (void)argc;

	int accel_axes[3];
	LSM303AGR_ACC_Get_Acceleration(NULL, accel_axes); //Read accelerometer
	chprintf(chp,"\rRaw: %d, %d, %d\r\n", accel_axes[0], accel_axes[1], accel_axes[2]);
	for(i = 0; i < 3; i++){
		accel_axes[i] = accel_axes[i] + calibrated_axes[i];
	}	
	chprintf(chp,"\rCalibrated: %d, %d, %d\r\n", accel_axes[0], accel_axes[1], accel_axes[2]);
}

static void cmd_display_counts(BaseSequentialStream *chp, int argc, char *argv[]){
	(void)argv;
	(void)argc; 

	int i;
	RTCDateTime ts;
	for(i = 0; i < num_timestamps; i++){
			ts = timestamp_data[i].time;
			int hours = ts.millisecond / 3600000; //converting ms to hours
			int mins = ts.millisecond % 3600000 / 60000; //converting ms to minutes
  		chprintf(chp,"\r%02d/%02d/%04d %02d:%02d -  %d\r\n", ts.month, ts.day,ts.year + 1980, hours, mins, timestamp_data[i].people_count);
	} 
  		chprintf(chp,"\r%d records, approx. %d seconds of recording\r\n", num_timestamps, num_timestamps * TIMESTAMP_FREQ);
}
//Recalibrates accelerometer
static void cmd_accel_calibrate(BaseSequentialStream *chp, int argc, char *argv[]){
	(void)argv;
	(void)argc; 
  		
	chprintf(chp,"\rDevice should be held in a fixed position in its intended orientation with no vibration\r\n");
	chprintf(chp,"\rCalibrating...\r\n");
	
	LSM303AGR_ACC_Get_Acceleration(NULL, calibrated_axes);
	chprintf(chp,"\rCalibrated with X: %d, Y: %d, Z:%d\r\n", calibrated_axes[0], calibrated_axes[1], calibrated_axes[2]);	
}

//Recalibrates accelerometer
static void cmd_start_tcp(BaseSequentialStream *chp, int argc, char *argv[]){
	(void)argv;
	(void)argc; 
  	
  	start_tcp = 1;	
	chprintf(chp, "\rSet TCP flag\r\n");
	
	
	}


//Sets current time in order to collect data with timestamps.
static void cmd_set_time(BaseSequentialStream *chp, int argc, char *argv[]){
	if(argc != 2){
		chprintf(chp,"\rIncorrect arguments. Example input:\r\n");		
		chprintf(chp,"\rset_time DD/MM/YYYY HH:MM (military time)\r\n");	
		return;	
	}

	char * token;
	RTCDateTime time_spec;
	const char slash[2] = "/";
	const char colon[2] = ":";

	//@TODO error handling for bad inputs
	token = strtok(argv[0], slash); //Day	
	time_spec.day = atoi(token); //Day
	token = strtok(NULL, slash); //Month	
	time_spec.month = atoi(token); //Month
	token = strtok(NULL, slash); //Year
	time_spec.year = atoi(token) - 1980; //Year (offset by 1980)
	
	int milliseconds;
	token = strtok(argv[1], colon); //Hour
	milliseconds = atoi(token) * 60 * 60 * 1000; //Hour to milliseconds
	token = strtok(NULL, colon); //Min
	time_spec.millisecond = atoi(token) * 60 * 1000 + milliseconds; //Min to millisecond

	//Fill in rest of struct with defaults
	time_spec.dstflag = 1;	
	time_spec.dayofweek = 1; //This is wrong

	rtcSetTime(&RTCD1, &time_spec);
	//"Wipe" old data.
	num_timestamps = 0;
	people_count = 0;
	start_time = time_spec;
	//@TODO make this better! Use system time
}


#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

static const ShellCommand commands[] = {
  {"myecho", cmd_myecho},
  {"accel_print", cmd_accel_print},
  {"accel_wai",cmd_accel_wai},
  {"accel_calibrate", cmd_accel_calibrate},
  {"display_counts", cmd_display_counts},
	{"d", cmd_display_counts}, //shortcut
	{"set_time", cmd_set_time},
	{"put_char", cmd_put_char},
	{"get_char", cmd_get_char},
	{"test_char", cmd_test_char},
	{"call_maggie", cmd_call_maggie},
	{"modem_cmd", cmd_modem_cmd},
	{"start_tcp", cmd_start_tcp},
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
  chprintf((BaseSequentialStream*)&SD5, "\rHint: run set_time DD/MM/YYYY HH:MM (military time) to initialize time.\r\n");


  /* Initialize the command shell */ 
  shellInit();

  /* Initialize the emulated UART */
	EUARTInit(&EUARTH); 

  /* Initialize the Accelerometer */
  LSM303AGR_ACC_Init();
  /* Calibrate the Accelerometer */
  LSM303AGR_ACC_Get_Acceleration(NULL, calibrated_axes);
  chprintf((BaseSequentialStream*)&SD5, "\rAccelerometer Calibrated\r\n");

  
  
	chThdCreateStatic(waCounterThread, sizeof(waCounterThread), NORMALPRIO+1, counterThread, NULL);
	chThdCreateStatic(waTimeStamp, sizeof(waTimeStamp), NORMALPRIO+1, timeStamp, NULL);
	chThdCreateStatic(waCheckThreshold, sizeof(waCheckThreshold), NORMALPRIO+1, checkThreshold, NULL);
	chThdCreateStatic(waTCPThread, sizeof(waTCPThread), NORMALPRIO+1, tcpThread, NULL);

while (TRUE) {
                if (!shelltp) {
                        shelltp = shellCreate(&shell_cfg1, SHELL_WA_SIZE, NORMALPRIO);
                }
                else if (chThdTerminatedX(shelltp)) {
                        chThdRelease(shelltp); /* Recovers memory of the previous shell.   */
                        shelltp = NULL; /* Triggers spawning of a new shell.        */
                }
		
	chThdSleepMilliseconds(1000);
      

}
}
