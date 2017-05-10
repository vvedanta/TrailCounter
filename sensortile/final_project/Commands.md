## Lab 4 Report
**Maggie Oates, moates**  
**Vinayak Vedantam, vvedanta**  
**03/23/2017**  

### Commands

#### Calibration
* Device should be held in a fixed position in its intended orientation with no vibration
* **accel_calibrate**: Will calibrate the accelerometer for future readings.

#### Accelerometer Commands
* **accel_wai**: Prints WHO_AM_I function results on the accelerometer
* **print**: Prints all recorded accelerometer readings

#### Getting Data
* Connect board to computer 
* *Start screen with logging dump*: screen -S board_data -L /dev/ttyUSB0 38400 
	* -L is logging flag. Will write all screen output to screenlog.n
	* -S names the screen session
* **display**: Prints out people counts alongside their respective timestamps
* *Retrieve data*: You can find this in screenlog.0
