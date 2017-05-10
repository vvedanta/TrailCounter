#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ch.h"
#include "test.h"
#include "chprintf.h"
#include "shell.h"
//#include "rtc.h"
//#include "counter_config.h"
#include "sim900.h"
char phone_number[10];
int * UART_address;

/*******************************
 * CELL FUNCTIONS
 *******************************/

static void send_SMS(char * message) {
	//AT 
	message;
}


static void send_count_data(struct time_data_t * counts_adr){
	counts_adr;
}

//Takes in a valid phone number
static void change_phone(char * new_num){
	new_num;
}

RTCDateTime get_time(){
	RTCDateTime local_time;

	return local_time;
}

void receive_SMS(){
//only for calibration.

}

void enable_low_power(){


}

void disable_low_power(){


}


