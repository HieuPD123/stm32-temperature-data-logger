#ifndef __SD_SPI_H
#define __SD_SPI_H

#include "main.h"
#include "string.h"

/* Return status */
#define SD_OK      0
#define SD_ERROR   1

/* Card types */
#define SD_TYPE_UNKNOWN  0
#define SD_TYPE_SDSC     1
#define SD_TYPE_SDHC     2

/* SD Commands */
#define CMD0    0
#define CMD1    1
#define CMD8    8
#define CMD9    9
#define CMD10   10
#define CMD12   12
#define CMD16   16
#define CMD17   17
#define CMD18   18
#define CMD24   24
#define CMD25   25
#define CMD55   55
#define CMD58   58

#define ACMD41  41

extern SPI_HandleTypeDef hspi1;

uint8_t SD_Init(void);

uint8_t SD_ReadBlock(
    uint8_t *buf,
    uint32_t sector);

uint8_t SD_WriteBlock(
    const uint8_t *buf,
    uint32_t sector);

uint8_t SD_ReadBlocks(
    uint8_t *buf,
    uint32_t sector,
    uint32_t count);

uint8_t SD_WriteBlocks(
    const uint8_t *buf,
    uint32_t sector,
    uint32_t count);

uint32_t SD_GetSectorCount(void);

#endif
