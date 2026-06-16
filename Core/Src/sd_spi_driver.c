#include "sd_spi_driver.h"

#define SD_CS_PORT GPIOA
#define SD_CS_PIN  GPIO_PIN_4

static uint8_t CardType = SD_TYPE_UNKNOWN;

static void SD_Select(void)
{
    HAL_GPIO_WritePin(
        SD_CS_PORT,
        SD_CS_PIN,
        GPIO_PIN_RESET);
}

static void SD_Deselect(void)
{
    HAL_GPIO_WritePin(
        SD_CS_PORT,
        SD_CS_PIN,
        GPIO_PIN_SET);
}


static uint8_t SPI_TxRx(uint8_t tx)
{
    uint8_t rx;

    HAL_SPI_TransmitReceive(
        &hspi1,
        &tx,
        &rx,
        1,
        HAL_MAX_DELAY);

    return rx;
}

static uint8_t SD_WaitReady(void)
{
    uint8_t resp;
    uint32_t timeout = HAL_GetTick();

    do
    {
        resp = SPI_TxRx(0xFF);

        if(resp == 0xFF)
        {
            return 1;
        }

    } while((HAL_GetTick() - timeout) < 500);

    return 0;
}

static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg)
{
    uint8_t crc;
    uint8_t resp;

    switch(cmd)
    {
        case CMD0:
            crc = 0x95;
            break;

        case CMD8:
            crc = 0x87;
            break;

        default:
            crc = 0x01;
            break;
    }

    /* 1 byte idle trước command */
    SPI_TxRx(0xFF);

    /* Command frame */
    SPI_TxRx(0x40 | cmd);

    SPI_TxRx((uint8_t)(arg >> 24));
    SPI_TxRx((uint8_t)(arg >> 16));
    SPI_TxRx((uint8_t)(arg >> 8));
    SPI_TxRx((uint8_t)arg);

    SPI_TxRx(crc);

    /* Chờ R1 response */
    for(uint8_t i = 0; i < 10; i++)
    {
        resp = SPI_TxRx(0xFF);

        if((resp & 0x80) == 0)
        {
            return resp;
        }
    }

    return 0xFF;
}

uint8_t SD_Init(void)
{
    uint8_t resp;
    uint32_t start;
    uint8_t r7[4];
    uint8_t ocr[4];

    CardType = SD_TYPE_UNKNOWN;

    // Power-up
    SD_Deselect();

    for(int i = 0; i < 10; i++)
        SPI_TxRx(0xFF);

    // Send CMD0
    SD_Select();
    start = HAL_GetTick();

    do
    {
        resp = SD_SendCmd(CMD0, 0);
        if(resp == 0x01)
            break;

    } while((HAL_GetTick() - start) < 1000);

    if(resp != 0x01)
    {
        goto error;
    }

    // Send CMD8
    resp = SD_SendCmd(CMD8, 0x1AA);

    // illegal Command
    if(resp == 0x05)
    {
        start = HAL_GetTick();

        // Send CMD55 -> ACMD41
        do
        {
            SD_SendCmd(CMD55, 0);
            resp = SD_SendCmd(ACMD41, 0x00000000);

            if(resp == 0)
            {
                CardType = SD_TYPE_SDSC;

                resp = SD_SendCmd(CMD16, 512);

                if(resp != 0x00)
                {
                    goto error;
                }

                break;
            }

        } while((HAL_GetTick() - start) < 1000);

        if(resp != 0)
        {
            goto error;
        }
    }
    else if(resp == 0x01) //Ver 2.0 or later SD Card
    {
        for(uint8_t i = 0; i < 4; i++)
        {
            r7[i] = SPI_TxRx(0xFF);
        }

        if((r7[2] != 0x01) || (r7[3] != 0xAA))
        {
            goto error;
        }

        start = HAL_GetTick();

        // Send CMD55 -> ACMD41 with HCS bit
        do
        {
            SD_SendCmd(CMD55, 0);
            resp = SD_SendCmd(ACMD41, 0x40000000U);

            if(resp == 0)
            {
                break;
            }

        } while((HAL_GetTick() - start) < 1000);

        if(resp != 0)
        {
            goto error;
        }

        resp = SD_SendCmd(CMD58, 0);

        if(resp != 0)
        {
            goto error;
        }

        for(uint8_t i = 0; i < 4; i++)
        {
            ocr[i] = SPI_TxRx(0xFF);
        }

        if(ocr[0] & 0x40)
        {
            CardType = SD_TYPE_SDHC;
        }
        else
        {
            CardType = SD_TYPE_SDSC;

            // Set block length = 512 bytes
            resp = SD_SendCmd(CMD16, 512);

            if(resp != 0x00)
            {
                goto error;
            }
        }
    }
    else
    {
        goto error;
    }

    SD_Deselect();
    SPI_TxRx(0xFF);

    return SD_OK;

error:
    SD_Deselect();
    SPI_TxRx(0xFF);
    CardType = SD_TYPE_UNKNOWN;
    return SD_ERROR;
}

uint8_t SD_ReadBlock(
    uint8_t *buf,
    uint32_t sector) {

	uint8_t resp;
	uint32_t addr;

	if (CardType == SD_TYPE_SDHC) {
		addr = sector;
	} else {
		addr = sector * 512;
	}

	SD_Select();

	if(!SD_WaitReady())
	{
	    SD_Deselect();
	    return SD_ERROR;
	}

	resp = SD_SendCmd(CMD17, addr);

	if (resp != 0x00)
	{
		SD_Deselect();
		return SD_ERROR;
	}

	uint32_t start = HAL_GetTick();

	while(SPI_TxRx(0xFF) != 0xFE)
	{
	    if((HAL_GetTick() - start) > 100)
	    {
	        SD_Deselect();
	    	return SD_ERROR;
	    }
	}

	static uint8_t dummy_tx[512];

	memset(dummy_tx, 0xFF, sizeof(dummy_tx));

	HAL_SPI_TransmitReceive(
	    &hspi1,
	    dummy_tx,
	    buf,
	    512,
	    HAL_MAX_DELAY);

	SPI_TxRx(0xFF); // CRC high
	SPI_TxRx(0xFF); // CRC low

	SD_Deselect();
	SPI_TxRx(0xFF);

	return SD_OK;
}

uint8_t SD_WriteBlock(
    const uint8_t *buf,
    uint32_t sector) {

	uint8_t resp;
	uint32_t addr;

	if (CardType == SD_TYPE_SDHC) {
		addr = sector;
	} else {
		addr = sector * 512;
	}

	SD_Select();

	if(!SD_WaitReady())
	{
	    SD_Deselect();
	    return SD_ERROR;
	}

	resp = SD_SendCmd(CMD24, addr);
	if (resp != 0x00)
	{
		SD_Deselect();
		return SD_ERROR;
	}

	// Start block token
	SPI_TxRx(0xFE);

	HAL_SPI_Transmit(
	    &hspi1,
	    (uint8_t*)buf,
	    512,
	    HAL_MAX_DELAY);

	SPI_TxRx(0xFF); // CRC high
	SPI_TxRx(0xFF); // CRC low

	resp = SPI_TxRx(0xFF);

	if ((resp & 0x1F) != 0x05)
	{
		SD_Deselect();
		SPI_TxRx(0xFF);
		return SD_ERROR;
	}

	SD_Deselect();
	SPI_TxRx(0xFF);
	return SD_OK;
}

uint8_t SD_ReadBlocks(
    uint8_t *buf,
    uint32_t sector,
    uint32_t count) {

	if (count < 1) {
		return SD_OK;
	}

	uint32_t read_index = 0;
	uint8_t resp;
	uint32_t addr;

	if (CardType == SD_TYPE_SDHC) {
		addr = sector;
	} else {
		addr = sector * 512;
	}

	SD_Select();

	if(!SD_WaitReady())
	{
	    SD_Deselect();
	    return SD_ERROR;
	}

	resp = SD_SendCmd(CMD18, addr);

	if (resp != 0x00)
	{
		SD_Deselect();
		return SD_ERROR;
	}

	for (uint32_t i = 0; i < count; i++)
	{
		uint32_t start = HAL_GetTick();

		while(SPI_TxRx(0xFF) != 0xFE)
		{
			if((HAL_GetTick() - start) > 100)
			{
				SD_Deselect();
				return SD_ERROR;
			}
		}

		static uint8_t dummy_tx[512];

		memset(dummy_tx, 0xFF, sizeof(dummy_tx));

		HAL_SPI_TransmitReceive(
		    &hspi1,
		    dummy_tx,
		    &buf[read_index],
		    512,
		    HAL_MAX_DELAY);

		read_index += 512;

		SPI_TxRx(0xFF); // CRC high
		SPI_TxRx(0xFF); // CRC low
	}

	resp = SD_SendCmd(CMD12, 0);
	if (resp != 0x00) {
		SD_Deselect();
	    return SD_ERROR;
	}

	if (!SD_WaitReady())
	{
		SD_Deselect();
	    return SD_ERROR;
	}

	SD_Deselect();
	SPI_TxRx(0xFF);

	return SD_OK;
}

uint8_t SD_WriteBlocks(
    const uint8_t *buf,
    uint32_t sector,
    uint32_t count)
{
    if (count < 1) {
        return SD_OK;
    }

    uint8_t resp;
    uint32_t addr;
    uint32_t write_index = 0;

    if (CardType == SD_TYPE_SDHC) {
        addr = sector;
    } else {
        addr = sector * 512;
    }

    SD_Select();

    if(!SD_WaitReady())
    {
        SD_Deselect();
        return SD_ERROR;
    }

    resp = SD_SendCmd(CMD25, addr);
    if (resp != 0x00)
    {
        SD_Deselect();
        return SD_ERROR;
    }

    for (uint32_t i = 0; i < count; i++)
    {
    	// Start block token
        SPI_TxRx(0xFC);

        HAL_SPI_Transmit(
            &hspi1,
            (uint8_t*)&buf[write_index],
            512,
            HAL_MAX_DELAY);

        write_index += 512;

        SPI_TxRx(0xFF); // CRC high
        SPI_TxRx(0xFF); // CRC low

        // Data Response token
        resp = SPI_TxRx(0xFF);

        // (0x05 = Data Accepted)
        if ((resp & 0x1F) != 0x05)
        {
            SD_Deselect();
            return SD_ERROR;
        }

        if(!SD_WaitReady())
        {
            SD_Deselect();
            return SD_ERROR;
        }
    }

    // Stop tran token
    SPI_TxRx(0xFD);

    SD_Deselect();

    SPI_TxRx(0xFF);

    return SD_OK;
}

uint32_t SD_GetSectorCount(void)
{
    uint8_t csd[16];
    uint8_t resp;
    uint32_t sectors = 0;

    SD_Select();

    if (!SD_WaitReady()) {
        SD_Deselect();
        return 0;
    }

    /* CMD9 (SEND_CSD)*/
    resp = SD_SendCmd(CMD9, 0);
    if (resp != 0x00) {
        SD_Deselect();
        return 0;
    }

    /* Data Token (0xFE) của khối dữ liệu CSD */
    uint32_t start = HAL_GetTick();
    while (SPI_TxRx(0xFF) != 0xFE) {
        if ((HAL_GetTick() - start) > 100) {
            SD_Deselect();
            return 0;
        }
    }

    /* Đọc 16 bytes dữ liệu của thanh ghi CSD */
    for (uint8_t i = 0; i < 16; i++) {
        csd[i] = SPI_TxRx(0xFF);
    }

    /* 2 bytes CRC */
    SPI_TxRx(0xFF);
    SPI_TxRx(0xFF);

    SD_Deselect();
    SPI_TxRx(0xFF); // dummy clock

    /* Bóc tách bit dựa vào phiên bản cấu trúc CSD (CSD_STRUCTURE nằm ở 2 bit cao của byte đầu tiên) */
    if ((csd[0] >> 6) == 1)
    {
        /* Thẻ SDHC hoặc SDXC (CSD Version 2.0) */
        /* C_SIZE nằm ở bit [69:48], tức là từ byte 7 đến byte 9 */
        uint32_t c_size = ((uint32_t)(csd[7] & 0x3F) << 16) | ((uint32_t)csd[8] << 8) | csd[9];
        sectors = (c_size + 1) * 1024;
    }
    else
    {
        /* Thẻ SDSC đời cũ (CSD Version 1.0) */
        /* C_SIZE nằm ở bit [73:62] */
        uint16_t c_size = ((uint16_t)(csd[6] & 0x03) << 10) | ((uint16_t)csd[7] << 2) | ((csd[8] & 0xC0) >> 6);
        /* C_SIZE_MULT nằm ở bit [49:47] */
        uint8_t c_size_mult = ((csd[9] & 0x03) << 1) | ((csd[10] & 0x80) >> 7);
        /* READ_BL_LEN nằm ở bit [83:80] */
        uint8_t read_bl_len = csd[5] & 0x0F;

        uint32_t block_len = 1 << read_bl_len;
        uint32_t mult = 1 << (c_size_mult + 2);
        uint64_t capacity = (uint64_t)(c_size + 1) * mult * block_len;

        sectors = (uint32_t)(capacity / 512); // Quy đổi ra số lượng sector 512-byte
    }

    return sectors;
}

