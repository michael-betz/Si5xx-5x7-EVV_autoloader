"""
Configure a Si5xx-5x7-EVB board to a specific frequency.
Board must run the `special` firmware.
"""
import serial, argparse
# from numpy import meshgrid, unravel_index

HS_DIVS = [4,5,6,7,None,9,None,11]
NS = [1]
NS.extend( range(2,130,2) )

def getDividers( f1 ):
    global HS_DIVS, NS
    for h in HS_DIVS[::-1]:
        for n in NS:
            if h is None:
                continue
            fDco = f1 * h * n
            # print( h, n, fDco/1e9 )
            if fDco>5.67e9:
                break;
            if fDco>=4.85e9:
                return h, n, fDco
    raise ValueError("Could not find a good combination of clock dividers")
    # HH, NN = meshgrid( [ x for x in HS_DIVS if x ], NS )
    # aDiffs = abs( args["f1"]*HH*NN - 4.85e9 )
    # inds = unravel_index( aDiffs.argmin(), aDiffs.shape )
    # return HS_DIVS[inds[0]], NS[inds[1]]

def main():
    parser = argparse.ArgumentParser( description=__doc__ )
    parser.add_argument(
        'f0',
        type = float,
        help =  "power-up frequency [Hz] of the Si570\
                (see http://www.silabs.com/products/timing/lookup-customize)"
    )
    parser.add_argument(
        'f1',
        type = float,
        help = "desired output frequency [Hz] (written to nonvolatile memory)"
    )
    parser.add_argument(
        '-p', '--port',
        default = "/dev/ttyUSB0",
        help = "serial port name"
    )
    args = parser.parse_args()
    print(args)

    with serial.Serial( args.port, timeout=3 ) as ser:
        ser.write(b'r')
        l = ser.readline().decode()     # 'r 01 C2 BC 81 83 02 \n'
    if not l.startswith('r '):
        raise RuntimeError("Unexpected response: " + l)
    print("Read: ", l.strip())
    ls = l.strip().split()[1:]          # ['01', 'C2', 'BC', '81', '83', '02']
    dats = [ int(x,16) for x in ls ]    # [1, 194, 188, 129, 131, 2]

    HS_DIV = HS_DIVS[ dats[0] >> 5 ]
    print("HS_DIV = ", HS_DIV)
    N1 = (dats[0]&0x1F)<<2 | dats[1]>>6
    N1 += 1
    # Illegal odd divider values will be rounded up to the nearest even value.
    if( (N1 > 1) and (N1 & 0x01) ):
        N1 += 1
    print("N1 = ", N1)
    RFFREQ = (dats[1]&0x3F)<<32 | dats[2]<<24 |         \
                    dats[3]<<16 | dats[4]<<8  | dats[5]
    RFFREQ /= 2**28
    print("RFFREQ = ", RFFREQ)

    fxtal = ( args.f0 * HS_DIV * N1 ) / RFFREQ
    print("fxtal = {:.6f} MHz".format(fxtal/1e6) )

    # Calculate new settings
    hs_div, n1, fDco = getDividers( args.f1 )
    rffreq = fDco / fxtal

    # Convert to registers again
    rffreq = int( rffreq * 2**28 )
    sets = bytearray(6)
    sets[5]  =  rffreq      & 0xFF
    sets[4]  = (rffreq>> 8) & 0xFF
    sets[3]  = (rffreq>>16) & 0xFF
    sets[2]  = (rffreq>>24) & 0xFF
    sets[1]  = (rffreq>>32) & 0xFF
    n1 -= 1
    sets[1] |= (    n1<< 6) & 0xFF
    sets[0]  = (    n1>> 2)
    hs_div = HS_DIVS.index( hs_div )
    sets[0] |= (hs_div<< 5) & 0xFF

    print( sets )

if __name__ == '__main__':
    main();
