#include "driver/spi.h"


/******************************************************************************
 * FunctionName : spi_master_init
 * Description  : SPI master initial function for common byte units transmission
 * Parameters   : uint8 spi_no - SPI module number, Only "SPI" and "HSPI" are valid
*******************************************************************************/
void ICACHE_FLASH_ATTR spi_master_init(uint8 spi_no, unsigned cpol, unsigned cpha, unsigned databits, uint32_t clock)
{
//	uint32 regvalue;

	if(spi_no>1) 		return; //handle invalid input number

	if(spi_no==SPI){
		WRITE_PERI_REG(PERIPHS_IO_MUX, 0x005); 
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CLK_U, 1);//configure io to spi mode
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CMD_U, 1);//configure io to spi mode	
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA0_U, 1);//configure io to spi mode	
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA1_U, 1);//configure io to spi mode	
	}
	else if(spi_no==HSPI){
		WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105); 
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2);//configure io to spi mode
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2);//configure io to spi mode	
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2);//configure io to spi mode	
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 2);//configure io to spi mode	
	}

	SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_CS_SETUP|SPI_CS_HOLD|SPI_DOUTDIN|SPI_USR_MOSI);

    //set clock polarity
    // TODO: This doesn't work
    //if (cpol == 1) {
    //    SET_PERI_REG_MASK(SPI_CTRL2(spi_no), (SPI_CK_OUT_HIGH_MODE<<SPI_CK_OUT_HIGH_MODE_S));
    //} else {
    //    SET_PERI_REG_MASK(SPI_CTRL2(spi_no), (SPI_CK_OUT_LOW_MODE<<SPI_CK_OUT_LOW_MODE_S));
    //}
    //os_printf("SPI_CTRL2 is %08x\n",READ_PERI_REG(SPI_CTRL2(spi_no)));

    //set clock phase
    if (cpha == 1) {
    	SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_CK_OUT_EDGE|SPI_CK_I_EDGE);
    } else {
    	CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_CK_OUT_EDGE|SPI_CK_I_EDGE);
    }

    CLEAR_PERI_REG_MASK(SPI_USER(HSPI), SPI_FLASH_MODE|SPI_WR_BYTE_ORDER|SPI_USR_MISO|
                        SPI_RD_BYTE_ORDER|SPI_USR_ADDR|SPI_USR_COMMAND|SPI_USR_DUMMY);

	//clear Daul or Quad lines transmission mode
	CLEAR_PERI_REG_MASK(SPI_CTRL(spi_no), SPI_QIO_MODE|SPI_DIO_MODE|SPI_DOUT_MODE|SPI_QOUT_MODE);

	// SPI clock=CPU clock/8
	WRITE_PERI_REG(SPI_CLOCK(spi_no), 
					((1&SPI_CLKDIV_PRE)<<SPI_CLKDIV_PRE_S)| \
					((3&SPI_CLKCNT_N)<<SPI_CLKCNT_N_S)| \
					((1&SPI_CLKCNT_H)<<SPI_CLKCNT_H_S)| \
					((3&SPI_CLKCNT_L)<<SPI_CLKCNT_L_S)); //clear bit 31,set SPI clock div

	//set 8bit output buffer length, the buffer is the low 8bit of register"SPI_FLASH_C0"
	WRITE_PERI_REG(SPI_USER1(spi_no), 
					((7&SPI_USR_MOSI_BITLEN)<<SPI_USR_MOSI_BITLEN_S)|
					((7&SPI_USR_MISO_BITLEN)<<SPI_USR_MISO_BITLEN_S));
}

/******************************************************************************
 * FunctionName : spi_master_deinit
 * Description  : SPI master deinitial function for common byte units transmission
 * Parameters   : uint8 spi_no - SPI module number, Only "SPI" and "HSPI" are valid
*******************************************************************************/
void ICACHE_FLASH_ATTR spi_master_deinit(uint8 spi_no)
{
  if(spi_no==HSPI){
//WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105); // WTF
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);//configure io to GPIO mode
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);//configure io to GPIO mode
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);//configure io to GPIO mode
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15);//configure io to GPIO mode
		GPIO_DIS_OUTPUT(FUNC_GPIO12);
		GPIO_DIS_OUTPUT(FUNC_GPIO13);
		GPIO_DIS_OUTPUT(FUNC_GPIO14);
		GPIO_DIS_OUTPUT(FUNC_GPIO15);
  }
}

/******************************************************************************
 * FunctionName : spi_mast_byte_write
 * Description  : SPI master 1 byte transmission function
 * Parameters   : 	uint8 spi_no - SPI module number, Only "SPI" and "HSPI" are valid
 *				uint8 data- transmitted data
*******************************************************************************/
void spi_mast_byte_write(uint8 spi_no, uint8 *data)
{
    if(spi_no>1) 		return; //handle invalid input number

    while(READ_PERI_REG(SPI_CMD(spi_no))&SPI_USR);

    WRITE_PERI_REG(SPI_W0(spi_no), *data);

    SET_PERI_REG_MASK(SPI_CMD(spi_no), SPI_USR);
    while(READ_PERI_REG(SPI_CMD(spi_no))&SPI_USR);

    *data = (uint8)(READ_PERI_REG(SPI_W0(spi_no))&0xff);
}  
/******************************************************************************
 * FunctionName : spi_mast_byte_write
 * Description  : SPI master 1 byte transmission function
 * Parameters   : 	uint8 spi_no - SPI module number, Only "SPI" and "HSPI" are valid
 *				uint8 data- transmitted data
*******************************************************************************/
uint8 HSPI_byte_io( uint8 data)
{
    while(READ_PERI_REG(SPI_CMD(HSPI))&SPI_USR);

    WRITE_PERI_REG(SPI_W0(HSPI), data);

    SET_PERI_REG_MASK(SPI_CMD(HSPI), SPI_USR);
    while(READ_PERI_REG(SPI_CMD(HSPI))&SPI_USR);

    return (uint8)(READ_PERI_REG(SPI_W0(HSPI))&0xff);
}
