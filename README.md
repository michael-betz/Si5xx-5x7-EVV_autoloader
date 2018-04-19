# Si5xx Autoloader
Alternative firmware for the `Si5xx-PROG_EVB` evaluation board, which turns it into a virtual USB serial port.
Register settings can be set manually with a serial terminal (like `miniterm.py`) or a target frequency can be programmed with the provided `setFreq.py`.
Settings are stored in non-volatile memory and applied to the Si5xx on power-up.

# Flashing the firmware
You'll need a [programming adapter](https://www.silabs.com/products/development-tools/mcu/8-bit/8bit-mcu-accessories/8-bit-debug-adapter).
Then either use the [production programmer](https://www.silabs.com/documents/login/software/MCUProductionProgrammer.zip), Silicon Labs IDE or Simplicity Studio. All 3 are free but need registration on the Silicon labs website.

# Setting a new frequency
Example of setting the output frequency to 500.1 MHz.

```bash
$ python setFreq.py -p /dev/ttyUSB1 156.25e6 500.1e6
Read initial register settings ...
     i 01 C2 BB FB FC AB --> HS_DIV: 4, N1: 8, RFFREQ: 43.749020260   f_xtal: 114.288274 MHz
Calculate new settings ...
     w E0 03 02 23 02 40 --> HS_DIV:11, N1: 1, RFFREQ: 48.133547068
Write to device and flash ...
     w E0 03 02 23 02 40 --> config_done
     r E0 03 02 23 02 40 --> verified
```

Note that 156.25 MHz is the `start-up frequency`, which can be looked up from the part-number on the
[Silabs website](https://www.silabs.com/products/timing/lookup-customize).

Example: [570aca000118](https://www.silabs.com/TimingUtility/timing-part-number-search-results.aspx?term=570aca000118).

# LEDs

At power-up, the two red LEDs [D3, D4] on the board indicate:

    [1,0] = Configuration over I2C in progress
    [0,0] = Configuration done, no I2C error
    [1,1] = I2C error

We found that the jumper J5 needs to be set for VCC > 1.8 V for reliable I2C communication.

# Development
... was done with Simplicity Studio 4 on a windows PC. The eclipse project is included in this git.

# Resources

 * [Si5xx-PROG_EVB evaluation board](https://www.silabs.com/documents/public/user-guides/Si5xx-PROG-EVB.pdf)
 * [Si570 Programmable XO](https://www.silabs.com/documents/public/data-sheets/si570.pdf)
 * [C8051F342 8-bit Microcontroller](https://www.silabs.com/documents/public/data-sheets/C8051F34x.pdf)
 * [Writing FLASH from firmware example](https://www.silabs.com/documents/public/example-code/AN201SW.zip)


