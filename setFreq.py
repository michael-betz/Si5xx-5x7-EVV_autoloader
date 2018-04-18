"""
Configure a Si5xx-5x7-EVB board to a specific frequency.
Board must run the `Si5xx-5x7-EVV_autoloader.hex` firmware
and visible as a virtual USB serial port.

Si570 datasheet:  https://www.silabs.com/documents/public/data-sheets/si570.pdf
"""
import serial, argparse
from Si570 import Si570

def getDividers( f1 ):
    """ returns a valid combination of clock dividers for target freq. `f1` """
    HS_DIVS = (4,5,6,7,9,11)
    NS = [1]
    NS.extend( range(2,130,2) )
    for h in HS_DIVS[::-1]:
        for n in NS:
            fDco = f1 * h * n
            # print( h, n, fDco/1e9 )
            if fDco>5.67e9:
                break;
            if fDco>=4.85e9:
                return h, n, fDco
    raise ValueError("Could not find a good combination of clock dividers")

def main():
    parser = argparse.ArgumentParser( description=__doc__ )
    parser.add_argument(
        'f0',
        type = float,
        help =  "power-up frequency [Hz] of the Si570,\
                 which corresponds to `Frequency A` on\
                 http://www.silabs.com/products/timing/lookup-customize"
    )
    parser.add_argument(
        'f1',
        type = float,
        help = "desired output frequency [Hz], which will be set on the device\
                and also written to nonvolatile memory"
    )
    parser.add_argument(
        '-p', '--port',
        default = "/dev/ttyUSB0",
        help = "serial port name"
    )
    args = parser.parse_args()

    print("Read initial register settings ... ")
    with serial.Serial( args.port, timeout=3 ) as ser:
        ser.write(b'i')
        l = ser.readline().decode()
    # l = 'i 01 C2 BC 81 83 02 \n'
    if not l.startswith('i '):
        raise RuntimeError("Unexpected response: " + l)

    # parse current settings
    sil = Si570( l );
    fxtal = sil.fxtal( args.f0 )
    print(
        "{:>24} --> {!s:<40} f_xtal: {:.6f} MHz".format(
            l.strip(), sil, fxtal/1.0e6
        )
    )

    # Calculate new settings
    hs_div, n1, fDco = getDividers( args.f1 )
    rffreq = fDco / fxtal

    # Un-parse new settings
    silNew = Si570();
    silNew.HS_DIV = hs_div
    silNew.N1 = n1
    silNew.RFFREQ = rffreq

    print("Calculate new settings ...")
    print("{a!r:>24} --> {a!s:<45}".format(a=silNew) )

    print("Write to device and flash ... ")
    with serial.Serial( args.port, timeout=3 ) as ser:
        ser.write( repr(silNew).encode("ascii") )
        print(
            "{:>24} --> {:}".format(
                ser.readline().decode().strip(),
                ser.readline().decode().strip()
            )
        )
        ser.write( b"r" )
        readBack = ser.readline().decode().strip()
        readBackOk = readBack[1:] == repr(silNew)[1:]
        print( "{:>24} --> {:}".format( readBack,  "verified" if readBackOk else "verify failed" ) )

if __name__ == '__main__':
    main();
