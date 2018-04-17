#ifndef I2C_H
#define I2C_H
#include <SI_C8051F340_Register_Enums.h>

// 16-bit SFR declarations
SBIT( LED0, SFR_P1, 0 );
SBIT( LED1, SFR_P1, 1 );
SBIT( SDA,  SFR_P0, 6 );
SBIT( SCL,  SFR_P0, 7 );

//--------------------------------------------------
// Some bit definition of the Silabs 570
//--------------------------------------------------
#define SI570_ADDR      	0x55		// Right shifted I2C address (MSB=0)
// REG_OFFS = 0x07 for all Si571 and Si570 with 20 ppm and 50 ppm temp. stab.
// REG_OFFS = 0x0D for Si570 with 7 ppm temp. stab.
#define REG_OFFS 			0x0D
#define CTRL_REG			135
#define SI570_RST_REG  		(1<<7)
#define SI570_NEW_FREQ 		(1<<6)
#define SI570_FREEZE_M 		(1<<5)
#define SI570_FREEZE_VCADC 	(1<<4)
#define SI570_RECALL 		(1<<0)
#define FREEZE_DCO_REG		137
#define SI570_FREEZE_DCO	(1<<4)

uint8_t i2c_write_regs( uint8_t i2cAddr, uint8_t regAddr, uint8_t *buffer, uint16_t len );
uint8_t i2c_write_reg( uint8_t i2cAddr, uint8_t regAddr, uint8_t val );
uint8_t i2c_read_regs( uint8_t i2cAddr, uint8_t regAddr, uint8_t *buffer, uint16_t len );
uint8_t i2c_read_reg( uint8_t i2cAddr, uint8_t regAddr );
void delayUs( volatile uint32_t x );

#endif
