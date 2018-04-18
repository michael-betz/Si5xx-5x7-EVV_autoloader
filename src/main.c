#include <stdint.h>
#include <string.h>
#include <SI_C8051F340_Register_Enums.h>
#include "VCPXpress.h"
#include "descriptor.h"
#include "F340_FlashPrimitives.h"
#include "i2c.h"

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
#define SYSCLK             6000000        // SYSCLK frequency in Hz
#define PACKET_SIZE        64

typedef enum {
    WAIT_FOR_W,
    WAIT_FOR_REGS,
    ALL_REGS_RECEIVED,
    PARSE_ERROR
} parseState;

// USB buffers
uint16_t xdata InCount;                   // Holds size of received packet
uint16_t xdata OutCount;                  // Holds size of transmitted packet
uint8_t  xdata RX_Packet[PACKET_SIZE];    // Packet received from host
uint8_t  xdata TX_Packet[PACKET_SIZE];    // Packet to transmit to host

const char code infoStr[] = "\n----------------------------------" \
                            "\n Magic Clock Box 3000     4/17/18 " \
                            "\n----------------------------------" \
                            "\ni = show Si570 power-up values"     \
                            "\nf = show flash values"              \
                            "\nr = read current values"            \
                            "\nw = write values ('w 01 c2 ...')\n\n";

// si570 reg buffers
uint8_t g_BuffWrite[6];                    // working buffer
uint8_t g_BuffInit[6];                     // backup of initial power up config

// Starting from `flash_start`, flash is used as eeprom
FLADDR flash_start = 0x5FF0;

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

void systemInit() {
    // Init. clock dividers / multipliers
    FLSCL  |= FLSCL_FLRT__SYSCLK_BELOW_25_MHZ;
    PFE0CN |= PFE0CN_PFEN__ENABLED;
    OSCICN |= OSCICN_IOSCEN__ENABLED
           | OSCICN_IFCN__HFOSC_DIV_1;           // Select full speed HFOSC
    CLKMUL  = 0x00;                              //clear multiplier
    CLKMUL |= CLKMUL_MULEN__ENABLED;             //enable multiplier
    delayUs( 200 );
    CLKMUL |= CLKMUL_MULINIT__SET;               //Initialize multiplier
    delayUs( 200 );
    while(!(CLKMUL & CLKMUL_MULRDY__BMASK));     // Wait for multiplier to lock
    CLKSEL = CLKSEL_CLKSL__DIVIDED_HFOSC;        // select IOSC (12 MHz)

    // Init. IO Ports
    P0MDOUT = 0x00;   // All P0 & P1 pins open-drain output
    P1MDOUT = 0x00;
    P0 = 0xFF;
    P2MDOUT   = P2MDOUT_B2__PUSH_PULL
                | P2MDOUT_B3__PUSH_PULL; // P2.2 - P2.3 are push-pull
    P2SKIP    = P2SKIP_B2__SKIPPED
                | P2SKIP_B3__SKIPPED;    // P2.2 - P2.3 skipped
    XBR1      = XBR1_XBARE__ENABLED;     // Enable the crossbar
}

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

// convert a buffer to a string of hex characters
// and write them to *out
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

// the `r` command: read the Si570 registers and print them
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
    //Freeze the DCO
    ret &= i2c_write_reg( SI570_ADDR, FREEZE_DCO_REG, SI570_FREEZE_DCO );
    //Write all 6 registers
    ret &= i2c_write_regs(SI570_ADDR, REG_OFFS, buf, 6 );
    //Unfreeze the DCO
    ret &= i2c_write_reg( SI570_ADDR, FREEZE_DCO_REG, 0 );
    //assert the NewFreq bit
    ret &= i2c_write_reg( SI570_ADDR, CTRL_REG, SI570_NEW_FREQ );
    return ret;
}

// write g_BuffWrite to permanent flash storage
void writeFlash(){
    FLADDR f = flash_start;
    uint8_t i, *p = g_BuffWrite;
    FLASH_PageErase(f);
    for( i=0; i<sizeof(g_BuffWrite); i++ ){
        FLASH_ByteWrite(f++, *p++);
    }
}

// restore g_BuffWrite from permanent flash storage
void readFlash(){
    FLADDR f = flash_start;
    uint8_t i, *p = g_BuffWrite;
    for( i=0; i<sizeof(g_BuffWrite); i++ ){
        *p++ = FLASH_ByteRead(f++);
    }
}

// the `w` command: write the entered data to chip and flash
void actionWriteRegs(){
    uint8_t ret=0, *p=TX_Packet;
    *p++ = 'w';
    *p++ = ' ';
    p = buf2hex( g_BuffWrite, sizeof(g_BuffWrite), p );
    writeFlash();
    ret = writeSi570Regs( g_BuffWrite );
    if( ret ){
        strcpy(p, "config_done\n");
    } else {
        strcpy(p, "i2c_err\n");
    }
}

// called before main()
void SiLabs_Startup (void){
   PCA0MD &= ~PCA0MD_WDTE__BMASK;   // Disable watchdog timer
}

// Gets called for each received character
// state-machine which handles the `w 01 02 ...` input command
// converts the hex characters to bytes and writes them to g_buffWrite
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
            // Check if we got a valid input character (0-F)
            if( temp <= 0x0F ){
                // if yes, write it nibble-wise to the reg-buffer
                if( currentNibble & 0x01 )
                    g_BuffWrite[ currentNibble/2 ] |= temp;
                else
                    g_BuffWrite[ currentNibble/2 ]  = temp << 4;
                currentNibble++;
            // if not, only tolerate (and ignore) certain characters,
            // otherwise cancel user input and throw an error
            } else {
                if( c==' ' || c==',' || c==';' ){
                    ;
                } else {
                    st = WAIT_FOR_W;
                    return PARSE_ERROR;
                }
            }
            // Check if we got all the characters needed
            if( currentNibble >= sizeof(g_BuffWrite)*2 ){
                st = WAIT_FOR_W;
                return ALL_REGS_RECEIVED;
            }
            break;

        default:
            st = WAIT_FOR_W;
    }
    return st;
}

// USB callback function
// runs in interrupt context
// called for every received / sent packet and when device is opened
VCPXpress_API_CALLBACK(usbCallback) {
    uint32_t INTVAL = Get_Callback_Source();
    uint8_t i, *p = TX_Packet;
    parseState st;
    if (INTVAL & DEVICE_OPEN){
        // Start first USB Read
        Block_Read(RX_Packet, PACKET_SIZE, &InCount);
    }
    // USB packet received ?
    if (INTVAL & RX_COMPLETE) {
        // For each character in the packet
        for (i=0; i<InCount; i++){
            // Hand it over to the parser (taking care of the `w` command)
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
                // Handle the single key commands
                if( RX_Packet[i] == 'i' ){
                    *p++ = 'i';
                    *p++ = ' ';
                    buf2hex( g_BuffInit, sizeof(g_BuffInit), p );
                    Block_Write(TX_Packet, strlen(TX_Packet), &OutCount);
                    return;
                }
                if( RX_Packet[i] == 'r' ){
                    actionReadRegs();
                    Block_Write(TX_Packet, strlen(TX_Packet), &OutCount);
                    return;
                }
                if( RX_Packet[i] == 'f' ){
                    readFlash();
                    *p++ = 'f';
                    *p++ = ' ';
                    buf2hex( g_BuffWrite, sizeof(g_BuffWrite), p );
                    Block_Write(TX_Packet, strlen(TX_Packet), &OutCount);
                    return;
                }
                if( RX_Packet[i] == '?' ){
                    Block_Write(infoStr, sizeof(infoStr), &OutCount);
                    return;
                }
            }
        }
        // Start next USB Read
        Block_Read(RX_Packet, PACKET_SIZE, &InCount);
    }
    // USB Write complete
    if (INTVAL & TX_COMPLETE){
        // Start next USB Read
        Block_Read(RX_Packet, PACKET_SIZE, &InCount);
    }
}

void main (void){
    uint8_t ret, temp=3;
    LED0 = 1;                           // write in progress = one led on
    LED1 = 0;
    systemInit();

    // Read Si570 initial reg values
    i2c_read_regs( SI570_ADDR, REG_OFFS, g_BuffInit, sizeof(g_BuffInit) );

    // Init the SI570 with data from flash
    readFlash();
    ret = writeSi570Regs( g_BuffWrite );

    USB_Init(&InitStruct);              // VCPXpress Initialization
    API_Callback_Enable( usbCallback ); // Enable VCPXpress API interrupts
    IE_EA = 1;                          // Enable global interrupts

    if( ret ){
        LED0 = 1;                       // i2c ok = both leds off
        LED1 = 1;
    } else {                            // i2c error = both leds on
        LED0 = 0;
        LED1 = 0;
    }
    while(1);
}
