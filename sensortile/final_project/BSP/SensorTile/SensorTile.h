/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SENSORTILE_H
#define __SENSORTILE_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* #define LSM303AGR_ACC_WHO_AM_I         0x33 */
/* #define LSM303AGR_MAG_WHO_AM_I         0x40 */
/* #define HTS221_WHO_AM_I_VAL         (uint8_t)0xBC */


uint8_t Sensor_IO_SPI_Write(SPIDriver *, SPIConfig *, uint8_t, uint8_t *, uint16_t );
uint8_t Sensor_IO_SPI_Read(SPIDriver *, SPIConfig *, uint8_t, uint8_t *, uint16_t );

#endif 
