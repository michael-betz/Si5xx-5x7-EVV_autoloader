/**************************************************************************//**
 * (C) Copyright 2014 Silicon Labs Inc,
 * http://developer.silabs.com/legal/version/v11/Silicon_Labs_Software_License_Agreement.txt
 *
 *****************************************************************************/

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <SI_C8051F340_Register_Enums.h>                // SI_SFR declarations
#include "VCPXpress.h"
#include "descriptor.h"
#include <stdint.h>
#include <string.h>
#include "i2c.h"
#include "F340_FlashPrimitives.h"



/**************************************************************************//**
 * VCPXpress_Echo_main.c
 *
 * Main routine for VCPXpress Echo example.
 *
 * This example simply echos any data received over USB back up to the PC.
 *
 *****************************************************************************/

//-----------------------------------------------------------------------------
// Global Constants
//-----------------------------------------------------------------------------
#define SYSCLK             6000000        // SYSCLK frequency in Hz
#define PACKET_SIZE 64
uint16_t xdata InCount;                   // Holds size of received packet
uint16_t xdata OutCount;                  // Holds size of transmitted packet
uint8_t xdata RX_Packet[PACKET_SIZE];     // Packet received from host
uint8_t xdata TX_Packet[PACKET_SIZE];     // Packet to transmit to host

typedef enum { WAIT_FOR_W, WAIT_FOR_REGS, ALL_REGS_RECEIVED, PARSE_ERROR } parseState;
uint8_t writeRegs[6];

FLADDR flash_start = 0x5FF0;			// For storing the reg vlaues to flash


// Function Prototypes
//-----------------------------------------------------------------------------
void Sysclk_Init (void);
static void Port_Init (void);

uint8_t hex2Nibble( uint8_t c ){
	if(  c>='A' && c<='F' ){
		return( c - 'A' + 10 );
	}
	if(  c>='a' && c<='f' ){
		return( c - 'a' + 10 );
	}
	if(  c>='0' && c<='9' ){
		return( c - '0' );
	}
	return 0xFF;
}

char nibble2hex( uint8_t v ){
	return "0123456789ABCDEF"[v & 0x0F];
}

uint8_t *buf2hex( uint8_t *buf, uint8_t n, uint8_t *out ){
	uint8_t temp;
	while( n-- > 0 ){
		temp = *buf++;
		*out++ = nibble2hex( temp>>4 );
		*out++ = nibble2hex( temp );
		*out++ = ' ';
	}
	*out++ = '\n';
	*out = 0;
	return out;
}

void actionReadRegs(){
	uint8_t ret;
	uint8_t readRegs[6], *p=TX_Packet;
	ret = i2c_read_regs( SI570_ADDR, REG_OFFS, readRegs, sizeof(readRegs) );
	*p++ = 'r';
	*p++ = ' ';
	if( ret ){
		buf2hex( readRegs, sizeof(readRegs), p );
	} else {
		strcpy(p, "i2c_err\n");
	}
}

// Rewrite all registers of Sil570. Takes care of freezing the DCO as well.
// *buf must be a 6 byte array.
uint8_t writeSi570Regs( uint8_t *buf ){
	uint8_t ret = 1;
	ret &= i2c_write_reg( SI570_ADDR, FREEZE_DCO_REG, SI570_FREEZE_DCO );	//Freeze the DCO
	ret &= i2c_write_regs(SI570_ADDR, REG_OFFS, buf, 6 );					//Write all 6 registers
	ret &= i2c_write_reg( SI570_ADDR, FREEZE_DCO_REG, 0 );					//Unfreeze the DCO
	ret &= i2c_write_reg( SI570_ADDR, CTRL_REG, SI570_NEW_FREQ );			//assert the NewFreq bit
	return ret;
}

void writeFlash(){
	FLADDR f = flash_start;
	uint8_t i, *p = writeRegs;
	FLASH_PageErase(f);
	for( i=0; i<sizeof(writeRegs); i++ ){
		FLASH_ByteWrite(f++, *p++);
	}
}

void readFlash(){
	FLADDR f = flash_start;
	uint8_t i, *p = writeRegs;
	for( i=0; i<sizeof(writeRegs); i++ ){
		*p++ = FLASH_ByteRead(f++);
	}
}

void actionWriteRegs(){
	uint8_t ret=0, *p=TX_Packet;
	*p++ = 'w';
	*p++ = ' ';
	p = buf2hex( writeRegs, sizeof(writeRegs), p );
	writeFlash();
	ret = writeSi570Regs( writeRegs );
	if( ret ){
		strcpy(p, "config_done\n");
	} else {
		strcpy(p, "i2c_err\n");
	}
}

//-----------------------------------------------------------------------------
// SiLabs_Startup() Routine
// ----------------------------------------------------------------------------
// This function is called immediately after reset, before the initialization
// code is run in SILABS_STARTUP.A51 (which runs before main() ). This is a
// useful place to disable the watchdog timer, which is enable by default
// and may trigger before main() in some instances.
//-----------------------------------------------------------------------------
void SiLabs_Startup (void){
   PCA0MD &= ~PCA0MD_WDTE__BMASK;             // Disable watchdog timer
}

// Gets called for each received character
parseState parseUserChar( uint8_t c ){
	static parseState st = WAIT_FOR_W;
	static uint8_t currentNibble = 0;
	uint8_t temp;
	switch( st ){
		case WAIT_FOR_W:
			if( c == 'w' ){
				currentNibble = 0;
				st = WAIT_FOR_REGS;
			}
			break;

		case WAIT_FOR_REGS:
			temp = hex2Nibble( c );
			if( temp <= 0x0F ){		// Check if we got a valid input character (0-F)
				if( currentNibble & 0x01 )
					writeRegs[ currentNibble/2 ] |= temp;
				else
					writeRegs[ currentNibble/2 ]  = temp << 4;
				currentNibble++;
			} else {				// if not, only tolerate (and ignore) certain characters, otherwise cancel input and throw error
				if( c==' ' || c==',' || c==';' ){
					;
				} else {
					st = WAIT_FOR_W;
					return PARSE_ERROR;
				}
			}
			if( currentNibble >= sizeof(writeRegs)*2 ){
				st = WAIT_FOR_W;
				return ALL_REGS_RECEIVED;
			}
			break;

		default:
			st = WAIT_FOR_W;
	}
	return st;
}

/**************************************************************************//**
 * @brief VCPXpress callback
 *
 * This function is called by VCPXpress. In this example any received data
 * is immediately transmitted to the UART. On completion of each write the
 * next read is primed.
 *
 *****************************************************************************/
VCPXpress_API_CALLBACK(usbCallback) {
	uint32_t INTVAL = Get_Callback_Source();
	uint8_t i, *p;
	parseState st;
	if (INTVAL & DEVICE_OPEN){
	  Block_Read(RX_Packet, PACKET_SIZE, &InCount);   // Start first USB Read
	}

	if (INTVAL & RX_COMPLETE) {                       // USB Read complete
	  for (i=0; i<InCount;i++){						  // For each received character in the usb packet
		 st = parseUserChar( RX_Packet[i] );
		 if( st == PARSE_ERROR ){
			 strcpy(TX_Packet, "inp_err\n");
			 Block_Write(TX_Packet, strlen(TX_Packet), &OutCount);
			 return;
		 } else if ( st == ALL_REGS_RECEIVED ) {
			 actionWriteRegs();
			 Block_Write(TX_Packet, strlen(TX_Packet), &OutCount);
			 return;
		 } else if( st == WAIT_FOR_W ) {
			 if( RX_Packet[i] == 'r' ){
				 actionReadRegs();
				 Block_Write(TX_Packet, strlen(TX_Packet), &OutCount);     // Start USB Write
				 return;
			 }
			 if( RX_Packet[i] == 'f' ){
				 readFlash();
				 p = TX_Packet;
				 *p++ = 'f';
				 *p++ = ' ';
				 buf2hex( writeRegs, sizeof(writeRegs), p );
				 Block_Write(TX_Packet, strlen(TX_Packet), &OutCount);     // Start USB Write
				 return;
			 }
			 if( RX_Packet[i] == '?' ){
				 strcpy(TX_Packet, "\nr = read\nw = write ('w 01 c2 bc 81 83 02')\nf = show flash\n");
				 Block_Write(TX_Packet, strlen(TX_Packet), &OutCount);     // Start USB Write
				 return;
			 }
		 }
	  }
	  Block_Read(RX_Packet, PACKET_SIZE, &InCount);   // Start next USB Read
	}

	if (INTVAL & TX_COMPLETE){                        // USB Write complete
	  Block_Read(RX_Packet, PACKET_SIZE, &InCount);   // Start next USB Read
	}
}

/**************************************************************************//**
 * @brief Main loop
 *
 * The main loop sets up the device and then waits forever. All active tasks
 * are ISR driven.
 *
 *****************************************************************************/
void main (void){
	uint8_t ret, temp=3;
	LED0 = 1;									// write in progress = one led on
	LED1 = 0;

	Sysclk_Init ();                            // Initialize system clock
	Port_Init ();                              // Initialize crossbar and GPIO

	readFlash();							   // Init the SI570 with data from flash
	ret = writeSi570Regs( writeRegs );

	USB_Init(&InitStruct);					   // VCPXpress Initialization
	API_Callback_Enable( usbCallback );		   // Enable VCPXpress API interrupts
	IE_EA = 1;       						   // Enable global interrupts

	if( ret ){
		LED0 = 1;							   // i2c ok = both leds off
		LED1 = 1;
	} else { 								   // i2c error = both leds on
		LED0 = 0;
		LED1 = 0;
	}
	while(1);
}


// Interrupt Service Routines
//-----------------------------------------------------------------------------



/**************************************************************************//**
 * @brief Clock initialization
 *
 * Setting SYSCLK to 12 MHz and USB to 48
 *****************************************************************************/
void Sysclk_Init (void)
{
  FLSCL |= FLSCL_FLRT__SYSCLK_BELOW_25_MHZ;
  PFE0CN |= PFE0CN_PFEN__ENABLED;

  OSCICN  |= OSCICN_IOSCEN__ENABLED
          | OSCICN_IFCN__HFOSC_DIV_1;           // Select full speed HFOSC

  CLKMUL = 0x00;                                //clear multiplier
  CLKMUL |= CLKMUL_MULEN__ENABLED;              //enable multiplier
  delayUs( 200 );
  CLKMUL |= CLKMUL_MULINIT__SET;                //Initialize multiplier
  delayUs( 200 );

  while(!(CLKMUL & CLKMUL_MULRDY__BMASK));      // Wait for multiplier to lock

  CLKSEL = CLKSEL_CLKSL__DIVIDED_HFOSC;         // select IOSC (12 MHz)
}

/**************************************************************************//**
 * @brief Port initialization
 *
 * P2.2   digital   push-pull    LED1
 * P2.3   digital   push-pull    LED2
 *
 *****************************************************************************/
static void Port_Init (void)
{
	P0MDOUT = 0x00;   // All P0 & P1 pins open-drain output
	P1MDOUT = 0x00;
	P0 = 0xFF;
   P2MDOUT   = P2MDOUT_B2__PUSH_PULL
               | P2MDOUT_B3__PUSH_PULL; // P2.2 - P2.3 are push-pull
   P2SKIP    = P2SKIP_B2__SKIPPED
               | P2SKIP_B3__SKIPPED;    // P2.2 - P2.3 skipped
   XBR1      = XBR1_XBARE__ENABLED;     // Enable the crossbar
}
