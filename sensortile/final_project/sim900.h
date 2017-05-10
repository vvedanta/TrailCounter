#include "rtc.h"
#include "counter_config.h"

/*******************************
 * CELL FUNCTIONS
 *******************************/

static void send_SMS(char * message);


static void send_count_data(struct time_data_t * counts_adr);

//Takes in a valid phone number
static void change_phone(char * new_num);

static RTCDateTime get_time();

void receive_SMS();

void enable_low_power();

void disable_low_power();


