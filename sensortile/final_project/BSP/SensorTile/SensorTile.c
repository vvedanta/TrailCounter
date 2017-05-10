/* Port of the Sensor IO low level functions for ChibiOS */
/* Bryce Himebaugh 2/28 */

#include "ch.h"
#include "hal.h"

#define SPI_1LINE_TX(__HANDLE__) ((__HANDLE__)->spi->CR1 |= (SPI_CR1_BIDIOE))
#define SPI_1LINE_RX(__HANDLE__) ((__HANDLE__)->spi->CR1 &= (~SPI_CR1_BIDIOE))
#define SPI_1LINE_ENABLE(__HANDLE__) ((__HANDLE__)->spi->CR1 |= SPI_CR1_SPE)
#define SPI_1LINE_DISABLE(__HANDLE__) ((__HANDLE__)->spi->CR1 &= (~SPI_CR1_SPE))
#define SPI_DMA_DISABLE(__HANDLE__) ((__HANDLE__)->spi->CR2 &= (~(SPI_CR2_RXDMAEN|SPI_CR2_TXDMAEN)))
#define SPI_DMA_ENABLE(__HANDLE__) ((__HANDLE__)->spi->CR2 |= (SPI_CR2_RXDMAEN|SPI_CR2_TXDMAEN))

uint8_t Sensor_IO_SPI_Write(SPIDriver *bus, SPIConfig *cfg, uint8_t WriteAddr, uint8_t *pBuffer, uint16_t nBytesToWrite ) {
  int i;

  if (nBytesToWrite > 1) {                /* Set bit 6 of the address if multiple bytes will be sent. */
    WriteAddr |= 0x40;
  }
  WriteAddr = WriteAddr & (~0x80);        /* Clear the write bit (bit 7)      */
  spiAcquireBus(bus);                     /* Acquire ownership of the bus.    */
  spiStart(bus, cfg);                     /* Setup transfer parameters.       */
  SPI_DMA_DISABLE(bus);                   /* Turn off the DMA interrupt since this bus is polled */
  SPI_1LINE_ENABLE(bus);
  SPI_1LINE_TX(bus);                      /* Make sure that the MOSI pin is output */
  spiSelect(bus);                         /* Slave Select assertion.          */
  spiPolledTx(bus, WriteAddr);            /* Send the address byte            */
  for(i=0;i<nBytesToWrite;i++) {
    spiPolledTx(bus, *pBuffer++);         /* Send the address byte            */
  }
  spiUnselect(bus);                       /* Slave Select de-assertion.       */
  spiReleaseBus(bus);                     /* Ownership release.               */
  return 0;
}

uint8_t Sensor_IO_SPI_Read(SPIDriver *bus, SPIConfig *cfg, uint8_t ReadAddr, uint8_t *pBuffer, uint16_t nBytesToRead ) {
  int i;
  if (nBytesToRead > 1) {                  /* Set bit 6 of the address if multiple bytes will be sent. */
    ReadAddr |= 0x40;
  }
  ReadAddr = ReadAddr | 0x80;             /* Set the read bit (bit 7)         */
  spiAcquireBus(bus);                     /* Acquire ownership of the bus.    */
  spiStart(bus, cfg);                     /* Setup transfer parameters.       */
  SPI_DMA_DISABLE(bus);                   /* Turn off the DMA interrupt since this bus is polled */
  SPI_1LINE_ENABLE(bus);
  spiSelect(bus);                         /* Slave Select assertion.          */
  spiPolledTx(bus, ReadAddr);             /* Send the address byte            */
  SPI_1LINE_DISABLE(bus);
  SPI_1LINE_RX(bus);
  for(i=0;i<nBytesToRead;i++) {
    *pBuffer++ = spiPolledRx(bus);     /* Receive data from the wire       */
  }
  spiUnselect(bus);                       /* Slave Select de-assertion.       */
  SPI_1LINE_TX(bus);
  spiReleaseBus(bus);                     /* Ownership release.               */
  return 0;
}

