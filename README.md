# [RollAByte](https://heap.45gfg9.net/rab-ex)

The same [*Roll a Byte*](https://heap.45gfg9.net/rants/809bc23b9ca6/), right in your hands this time.

This is a simple device to produce a random byte and show it on a 72x40 OLED display and 8 LEDs, powered by an ATmega88PA and the amazing [U8g2](https://github.com/olikraus/u8g2) library.

## Build

This repository is a PlatformIO project. To build the project, clone this repository and open in PlatformIO IDE / build with PlatformIO CLI.

```bash
git clone --recurse-submodules https://github.com/45gfg9/RollAByte.git
cd RollAByte
pio run
```

The fuse settings are set to use the internal 8MHz oscillator with the CKDIV8 fuse disabled. For ATmega88PA, the fuses are set as follows:

- Low: `0xe2`
- High: `0xdf`
- Extended: `0xff`

```bash
avrdude -pm88p -cusbasp -Ulfuse:w:0xe2:m -Uhfuse:w:0xdf:m -Uefuse:w:0xff:m
```

## Hardware

The schematic, PCB layout and EasyEDA source are available in the `hardware` directory.

![Schematic](hardware/RollAByte_v2_schematic.svg)

![PCB front](hardware/RollAByte_v2_PCB_front.svg)

![PCB back](hardware/RollAByte_v2_PCB_back.svg)

## License

This project is licensed under the WTFPL license. See the [LICENSE](LICENSE) file for details.

The font used in the display is [fcambus/spleen](https://github.com/fcambus/spleen), which is licensed under the BSD 2-Clause license.
