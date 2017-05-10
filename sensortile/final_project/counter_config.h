#include "rtc.h"

/*******************************
 * THE HEADER FILE
 *******************************/

// struct that maggie will comment on extensively
struct time_data_t{
  RTCDateTime time; //timestamp (see rtc.h in Chibi)
  int people_count; //number of passerbys in that minute
  					//-1 denotes a re-calibration because no counts have been read
  					//-2 denotes a recalibration because too many counts have been read
};

//Accelerometer theshold for detecting pedestrian movement
#define MOVEMENT_THRESHOLD 500 // (in mG)

//How often to check for pedestrian movement
#define ACCEL_MOVEMENT_FREQ 1000 // (in milliseconds)

//If no counts have been read in a certain time, try recalibrating
//This value is approximate & should be roughly on the same order of seconds as
// TIMESTAMP_FREQ
#define RECALIBRATION_FREQ 600 //in seconds

 //Default accelerometer calibration initialization.
 //Z-axis is 1 G to offset gravity
 //static int DEFAULT_AXES[3] = {0, 0, 1000};

//Maximum number of timestamps to write
//@TODO add bounds check
#define MAX_TIMESTAMPS 500

//How often to write people counts. (How much to aggregate data)
#define TIMESTAMP_FREQ 15 //in seconds

//How often to send data via sms.
#define EXPORT_FREQ 15 //in seconds

// Space saving mode.
// Value: 	Function:
//  0		Stores value for every timestamp
//	1		Only stores pedestrian count if count is nonzero
#define SS_FLAG 0

// TCP Information
//How often to send data via TCP
#define TCP_FREQ 60 //in seconds 600
// PORT
#define PORT_NUMBER 9999
// HOST
#define SERVER_ADDRESS "129.79.242.93"
// APN
#define TPN_APN "tpo.pmvne"
