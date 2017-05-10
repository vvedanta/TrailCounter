#include "ch.h"
#include "chmtx.h"

/* CONSTANTS */
#define EUART_WORD_SIZE 8               //plus one stop/start bit? bits per "frame"
#define EUART_BAUD 1200                //bps
#define EUART_INTERRUPT_MODE 0  //flag. 1 = use hardware interrupts
#define EUART_TX_PORT GPIOC          //pointer to tx pin to manipulate //GPIO2/SB16/CN5 9/PC1 F8 ??
#define EUART_TX_PIN 1          //pointer to tx pin to manipulate //GPIO2/SB16/CN5 9/PC1 F8 ??
#define EUART_RX_PORT GPIOC          //pointer to rx pin to watch    //GPIO3/SB17/CH5 10/PC0 F9 ??
#define EUART_RX_PIN 0          //pointer to rx pin to watch    //GPIO3/SB17/CH5 10/PC0 F9 ??
#define EUART_MAX_TIME 5000      //timeout limit milisec?


/* STRUCTS */
typedef struct CircularQueue{
        uint8_t * head; //aka char
        uint8_t * tail; //aka char
        uint8_t size;   //max size
} CircularBuffer;

typedef struct EmulatedUARTHandle{
        uint8_t WordSize;
        uint32_t BaudRate;
	uint16_t TimerPeriod; 	//microseconds
	uint16_t TimeOut; 	//milliseconds
        CircularBuffer * txBuffer;
        CircularBuffer * rxBuffer;
        uint8_t txXferCount;    //Bytes transmitted "so far"
        uint8_t rxXferCount;    //Bytes received "so far"
        uint8_t uartStatus;     //a EUART status code
//        Mutex * txLock;
//        Mutex * rxLock;
} EUARTHandle;


// EUART STATUS CODES // 
#define EUART_UNINIT  0x00 //Not initialized!
#define EUART_READY      0x01
#define EUART_BUSY       0x02
#define EUART_BUSY_TX    0x03
#define EUART_BUSY_RX    0x04
#define EUART_BUSY_RXTX  0x05
#define EUART_TIMEOUT    0x06
#define EUART_ERROR      0x07



/* FUNCTION DECs */
int EUART_Init_Pins(void);
//void EUART_Init(EUARTHandle * handle, Mutex * rxLock, Mutex * txLock);
void EUARTInit(EUARTHandle * handle);
int EUARTPutChar(EUARTHandle * handle, char ch);
uint8_t EUARTGetChar(EUARTHandle * handle, char * ch);
//int EUART_Transmit(EUARTHandle * handle, CircularBuffer * txBuffer, uint16_t txXferSize);
//int EUART_Receieve(EUARTHandle * handle, CircularBuffer * rxBuffer, uint16_t rxXferSize);

