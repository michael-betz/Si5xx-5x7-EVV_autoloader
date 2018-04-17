# Si5xx Autoloader
Alternative firmware for the `Si5xx-PROG_EVB` evaluation board, which turns it into a virtual USB serial port.
Register settings can be set manually with `miniterm.py` or a target frequency can be programmed with the provided `setFreq.py`.
Settings are stored in non-volatile memory and applied to the Si5xx on power-up.

# Flashing the firmware
either use the [production programmer](https://www.silabs.com/documents/login/software/MCUProductionProgrammer.zip), Silicon Labs IDE or Simplicity Studio. All 3 are free but need registration on the Silicon labs website. Alternatively, bring the board to `46-146c` ;)

# Setting a new frequency
Example of setting the output frequency to 500.1 MHz.

```bash
$ python setFreq.py -p /dev/ttyUSB1 156.25e6 500.1e6
Read initial register settings ...
     i 01 C2 BC 81 83 02 --> HS_DIV: 4, N1: 8, RFFREQ: 43.781619079
Calculate new settings ...
     w e0 03 02 b5 ea 58 --> HS_DIV:11, N1: 1, RFFREQ: 48.169412941
Write to device and flash ...
     w E0 03 02 B5 EA 58 --> config_done
     r E0 03 02 B5 EA 58
```

Note that 156.25 MHz is the `start-up frequency`, which can be looked up from the part-number on the
[Silabs website](https://www.silabs.com/products/timing/lookup-customize).

For the example part [these are the results](https://www.silabs.com/TimingUtility/timing-part-number-search-results.aspx?term=570aca000118).

# Development
... was done with Simplicity Studio 4 on a windows PC. The eclipse project is included in this git.

# Resources

 * [Si5xx-PROG_EVB evaluation board](https://www.silabs.com/documents/public/user-guides/Si5xx-PROG-EVB.pdf)
 * [Si570 Programmable XO](https://www.silabs.com/documents/public/data-sheets/si570.pdf)
 * [C8051F342 8-bit Microcontroller](https://www.silabs.com/documents/public/data-sheets/C8051F34x.pdf)
 * [Writing FLASH from firmware example](https://www.silabs.com/documents/public/example-code/AN201SW.zip)


