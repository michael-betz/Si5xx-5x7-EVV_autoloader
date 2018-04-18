class Si570( object ):
    """
    helper class to convert from numbers used in the datasheet
    to register values
    """
    HS_DIVS_LOOKUP = (4,5,6,7,None,9,None,11)

    def __init__(self, l=None):
        """
        l can be a string like 'r 01 C2 BC 81 83 02 \n'
        """
        if l:
            ls = l.strip().split()[1:] # ['01', 'C2', 'BC', '81', '83', '02']
            self._regs = bytearray([ int(x,16) for x in ls ])
        else:
            self._regs = bytearray(6);

    @property
    def HS_DIV(self):
        """ DCO High Speed Divider """
        return Si570.HS_DIVS_LOOKUP[ self._regs[0] >> 5 ]

    @HS_DIV.setter
    def HS_DIV(self, value):
        ind = Si570.HS_DIVS_LOOKUP.index(value)
        self._regs[0] &= 0x1F
        self._regs[0] |= (ind<<5) & 0xE0

    @property
    def N1(self):
        """ CLKOUT Output Divider """
        N1 = (self._regs[0]&0x1F)<<2 | self._regs[1]>>6
        N1 += 1
        # Illegal odd divider values will be rounded up to the nearest even value.
        if( (N1 > 1) and (N1 & 0x01) ):
            print( "Illegal N1: {0}, rounding up to {1}", N1, N1+1 )
            N1 += 1
        return N1

    @N1.setter
    def N1(self, value):
        value -= 1
        self._regs[0] &= 0xE0
        self._regs[0] |= (value>>2) & 0x1F
        self._regs[1] &= 0x3F
        self._regs[1] |= (value<<6) & 0xC0

    @property
    def RFFREQ(self):
        """ Reference Frequency control input to DCO. [float] """
        RFFREQ = (self._regs[1]&0x3F)<<32 | self._regs[2]<<24 |         \
                        self._regs[3]<<16 | self._regs[4]<<8  | self._regs[5]
        return RFFREQ / 2.0**28

    @RFFREQ.setter
    def RFFREQ(self, value):
        value = int( value * 2**28 )
        self._regs[1] &= 0xC0
        self._regs[1] |= (value>>32) & 0x3F
        self._regs[2]  = (value>>24) & 0xFF
        self._regs[3]  = (value>>16) & 0xFF
        self._regs[4]  = (value>> 8) & 0xFF
        self._regs[5]  =  value      & 0xFF

    def fxtal(self, f0):
        """
        calculate internal crystal frequency.
        f0 = startup frequency (see http://www.silabs.com/products/timing/lookup-customize)
        """
        return ( f0 * self.HS_DIV * self.N1 ) / self.RFFREQ

    def __repr__(self):
        """ register values as hex string like `w E0 03 02 B5 EA 58` """
        s  = 'w '
        s += ' '.join('{:02X}'.format(x) for x in self._regs)
        return s

    def __str__(self):
        """ decoded register values """
        s = "HS_DIV:{0:2d}, N1:{1:2d}, RFFREQ:{2:13.9f}".format( self.HS_DIV, self.N1, self.RFFREQ )
        return s
