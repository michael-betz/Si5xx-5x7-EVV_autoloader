"""
Configure a Si5xx-5x7-EVB board to a specific frequency.
Board must run the `Si5xx-5x7-EVV_autoloader.hex` firmware
and visible as a virtual USB serial port.

Si570 datasheet:  https://www.silabs.com/documents/public/data-sheets/si570.pdf
"""
import sys
import serial
import argparse
from Si570 import calcFreq


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        'f0',
        type=float,
        help="power-up frequency [Hz] of the Si570,\
                 which corresponds to `Frequency A` on\
                 http://www.silabs.com/products/timing/lookup-customize"
    )
    parser.add_argument(
        'f1',
        type=float,
        help="desired output frequency [Hz], which will be set on the device\
                and also written to nonvolatile memory"
    )
    parser.add_argument(
        '-p', '--port',
        default="/dev/ttyUSB0",
        help="serial port name"
    )
    args = parser.parse_args()

    print("Read initial register settings ... ")
    with serial.Serial(args.port, timeout=3) as ser:
        ser.write(b'i')
        ll = ser.readline().decode().strip()
    # ll = 'i 01 C2 BB FB FC AB'
    if not ll.startswith('i '):
        raise RuntimeError("Unexpected response: " + ll)

    silNew = calcFreq(ll, args.f0, args.f1)

    print("Calculate new settings ...")
    print("{a!r:>24} --> {a!s:<45}".format(a=silNew))

    print("Write to device and flash ... ")
    with serial.Serial(args.port, timeout=3) as ser:
        ser.write(repr(silNew).encode("ascii"))
        print(
            "{:>24} --> {:}".format(
                ser.readline().decode().strip(),
                ser.readline().decode().strip()
            )
        )
        ser.write(b"r")
        readBack = ser.readline().decode().strip()
        readBackOk = readBack[1:] == repr(silNew)[1:]
        print("{:>24} --> {:}".format(
            readBack, "verified" if readBackOk else "verify failed"
        ))
        if not readBackOk:
            sys.exit(1)


if __name__ == '__main__':
    main()
