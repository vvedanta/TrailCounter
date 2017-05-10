#include "emu_uart.h"
#include "ch.h"
#include "hal.h"
#include "chmtx.h"
#include "chvt.h"

//Initializes TX/RX pins
int EUARTInitPins(void){

	palSetPadMode(EUART_RX_PORT, EUART_RX_PIN, PAL_MODE_INPUT); 		//Initialize RX
	palSetPadMode(EUART_TX_PORT, EUART_TX_PIN, PAL_MODE_OUTPUT_PUSHPULL);	//Initialize TX

	palSetPad(EUART_TX_PORT, EUART_TX_PIN);  //Set high
	
	return 0;
}

//Inputs: empty EUARTHandle
void EUARTInit(EUARTHandle * handle){
	handle->uartStatus = EUART_UNINIT; 
	handle->WordSize = EUART_WORD_SIZE;
	handle->BaudRate = EUART_BAUD;
	handle->TimeOut = EUART_MAX_TIME;	//milliseconds
	handle->TimerPeriod = 1000000 / EUART_BAUD; //microseconds per bit
	//handle->TimerPeriod = 833;
 	handle->txXferCount = 0;
	handle->rxXferCount = 0;
	EUARTInitPins();
	handle->uartStatus = EUART_READY; //We're initialized!
}
/*
//Inputs: empty EUARTHandle, two Mutex pointers that will wrap rx/tx buffers
void EUARTInit(EUARTHandle * handle, Mutex * rxLock, Mutex * txLock){
	handle->uartStatus = EUART_UNINIT; 
	handle->WordSize = EUART_WORD_SIZE;
	handle->BaudRate = EUART_BAUD;
	handle->TimerPeriod = 1000000 / EUART_BAUD ; //Microseconds!
	handle->TimeOut = EUART_TIMEOUT;
	handle->txXferCount = 0;
	handle->rxXferCount = 0;
	handle->txLock = txLock;
	handle->rxLock = rxLock;
	EUART_Init_Pins();
	handle->uartStatus = EUART_READY; //We're initialized!
}
*/


//return 0 if no probs
int EUARTPutChar(EUARTHandle * handle, char ch){
	
	int bit_num;
	uint8_t byte = (uint8_t) ch;
	systime_t start, period;

	start = chVTGetSystemTime();
	period = US2ST(handle->TimerPeriod);

	//start bit
	palClearPad(EUART_TX_PORT, EUART_TX_PIN);
	chThdSleepUntil(start + period);
	

	for(bit_num = 0; bit_num < EUART_WORD_SIZE; bit_num++){

			//iterate through char bits
			if(byte & 0x01){
				palSetPad(EUART_TX_PORT, EUART_TX_PIN);		}	//set High
			else{
				palClearPad(EUART_TX_PORT, EUART_TX_PIN);			}	//set Low
				byte = byte >> 1; 	

		chThdSleepUntil(start + ((bit_num+2) * period));

		}
	

	//stop bit
	palSetPad(EUART_TX_PORT, EUART_TX_PIN);
	chThdSleep(period);
	return 0;
}

uint8_t EUARTSendCommand(EUARTHandle * handle, char * cmd, int length){
	int ch_i;
	for(ch_i = 0; ch_i < length; ch_i++){	
		EUARTPutChar(handle, cmd[ch_i]);
	}

	EUARTPutChar(handle, '\r');
	return 0;
}



//Takes output letter, returns an EUART status code (see euart.h)
uint8_t EUARTGetChar(EUARTHandle * handle, char * letter){
	int bit_num;
	int sample_num;
	uint8_t byte = 0x00;
	uint8_t sample_accum = 0;
	systime_t start, sample_period, period, timeout;
	uint8_t timedOut = 0; //Flag

	start = chVTGetSystemTime();
	timeout = start + MS2ST(handle->TimeOut);
	while(palReadPad(EUART_RX_PORT, EUART_RX_PIN) && !timedOut){ //either detect start pin or give up & timeout
		timedOut =  !chVTIsSystemTimeWithin(start,timeout);
	}
	
	//check for timeout
	if(timedOut) {
		*letter = '\0'; 
		return EUART_TIMEOUT; }

	//wait half-cycle, 
	start = chVTGetSystemTime();
	period = US2ST(handle->TimerPeriod); //Read 3/bit to check for offset errors
	sample_period = period / 4;
	chThdSleepMicroseconds(period+ sample_period); //offset cycle slightly
	
	//Read 8 bits
	for(bit_num = 1; bit_num < EUART_WORD_SIZE+1; bit_num++){ //one-indexing for easier math
		chSysLock();	
		for(sample_num = 1; sample_num < 4; sample_num++){
			//Read pin 3x per bit to reduce error
			sample_accum += palReadPad(EUART_RX_PORT, EUART_RX_PIN);
			chThdSleepMicroseconds(sample_period);
		}

		//interpreting samples	
		byte = byte | (sample_accum / 2); //Out of 3 samples, use majority yay integer division
		if(bit_num < EUART_WORD_SIZE){byte = byte << 1;} //pack byte, don't shift last val
		sample_accum = 0;

		chSysUnlock();	
		//sleep until next bit
		chThdSleepUntil(start + (bit_num * period));
	}

	chThdSleepMicroseconds(sample_period); //give ourselves a little cushion.	
	if(!palReadPad(EUART_RX_PORT, EUART_RX_PIN)){ 
		//We're missing our stop bit :(
		//pack what we have & throw our error
		*letter = (char) byte;
		return EUART_ERROR; 
	}
	else{ 
		*letter = (char) byte;	
		return EUART_READY;
	}
}


/*
//returns 1 for error
int EUART_Transmit(EUARTHandle * handle, CircularBuffer * txBuffer, uint16_t txXferSize){
	if(handle->uartStatus == EUART_READY || handle == EUART_BUSY_RX){
		chMtxLock(handle->txLock); //I don't 100% understand how these work
		
		if(txXferSize > txBuffer.size){
			//Bad args
			handle->uartStatus = EUART_ERROR;
			return 1;
		}

		handle->txBuffer = txBuffer;
		handle->txXferCount = 0; //reset transfer count to 0		

		//state transitions
		if(handle->uartStatus == EUART_BUSY_RX){
			handle->uartStatus = EUART_BUSY_RXTX;
		}
		else{
			handle->uartStatus = EUART_BUSY_TX;
		}
	
		//drop to signal start.

		//start writing
		while(handle->txXferCount < txXfersize && handle->txBuffer->tail > handle->txBuffer->head){

			//check for timeout as todo

			//pull characters by moving head around.
			uint8_t a_char = *(txBuffer->tail);
			uint8_t bit_count;
			for(bit_count = 0; bit_count < 8; bit_count++){
				//need to transmit at baud rate
				//use chvt? 
				
				//check endianness?
				//do some masking & use BSRR? 
			} 
		}
		//raise stop bit @TODO
		
		//state transitions
		if(handle->uartStatus == EUART_BUSY_RXTX){
			handle->uartStatus = EUART_BUSY_RX;
		}
		else{
			handle->uartStatus = EUART_READY;
		}
		chMtxUnlock(handle->txLock);
		return 0;
	}
	//Already transmitting
	return 1;
};*/

int EUART_Receive(EUARTHandle * handle, CircularBuffer * rxBuffer, uint16_t rxXferSize);


