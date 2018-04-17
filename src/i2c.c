//-------------------------------------------------
// The (in)famous picorv32soc Soft-I2C library
//-------------------------------------------------
#include "i2c.h"
#include <stdint.h>

// setting / clearing / reading the I2C pins
#define SDA1() { SDA=1; I2C_DELAY(); }  // SDA SET 1
#define SDA0() { SDA=0; I2C_DELAY(); }
#define SDAR()   SDA                    // SDA READ
#define SCL1() { SCL=1; I2C_DELAY(); }  // SCL SET 1
#define SCL0() { SCL=0; I2C_DELAY(); }
#define I2C_DELAY() 		 delayUs( I2C_DELAY_US )
#define I2C_DELAY_US 		 1
#define I2C_R                1
#define I2C_W                0
#define I2C_ACK              1
#define I2C_NACK             0

void delayUs( volatile uint32_t x ){
	volatile uint8_t temp = 0;
	while( x-- > 0 ){
		temp = 50;
		while( temp-- );
	}
}

//-------------------------------------------------
// Low level I2C functions
//-------------------------------------------------
void i2c_stop(){
    SDA0();
    SCL1();
    SDA1();
}

void i2c_start(){
    SCL1();
    SDA0();
    SCL0();
}

uint8_t i2c_tx( uint8_t dat ){
	uint8_t i, ack;
    for ( i=0; i<=7; i++ ){
        if ( dat & 0x80 ){
            SDA1();
        } else {
            SDA0();
        }
        SCL1();
        dat <<= 1;
        SCL0();
    }
    // Receive ack from slave
    SDA1();
    SCL1();
    ack = SDAR() == 0;
    SCL0();
    return ack;
}

uint8_t i2c_rx( uint8_t ack ){
    uint8_t i, dat = 0;
    // TODO check for clock stretching here
    for ( i=0; i<=7; i++ ){
        dat <<= 1;
        SCL1();
        dat |= SDAR();
        SCL0();
    }
    // Send ack to slave
    SCL0();
    if ( ack ){
        SDA0();
    } else {
        SDA1();
    }
    SCL1();
    SCL0();
    SDA1();
    return dat;
}

//-------------------------------------------------
// High level I2C functions (dealing with registers)
//-------------------------------------------------
uint8_t i2c_write_regs( uint8_t i2cAddr, uint8_t regAddr, uint8_t *buffer, uint16_t len ){
    uint8_t ret=1;
    i2c_start();
    ret &= i2c_tx( (i2cAddr<<1) | I2C_W );
    ret &= i2c_tx( regAddr );
    while ( len-- > 0 ){
        ret &= i2c_tx( *buffer++ );
    }
    i2c_stop();
    return ret;
}

uint8_t i2c_write_reg( uint8_t i2cAddr, uint8_t regAddr, uint8_t val ){
	return i2c_write_regs( i2cAddr, regAddr, &val, 1 );
}

uint8_t i2c_read_regs( uint8_t i2cAddr, uint8_t regAddr, uint8_t *buffer, uint16_t len ){
    uint8_t ret=1;
    i2c_start();
    ret &= i2c_tx( (i2cAddr<<1) | I2C_W );
    ret &= i2c_tx( regAddr );
    i2c_start();                        // Repeated start to switch to read mode
    ret &= i2c_tx( (i2cAddr<<1) | I2C_R );
    while ( len-- > 0 ){
        *buffer++ = i2c_rx( len!=0 );   // Send NACK for the last byte
    }
    i2c_stop();
    return ret;
}

uint8_t i2c_read_reg( uint8_t i2cAddr, uint8_t regAddr ){
	uint8_t temp;
	i2c_read_regs( i2cAddr, regAddr, &temp, 1 );
	return temp;
}
