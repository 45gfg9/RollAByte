# [RollAByte](https://heap.45gfg9.net/rab-ex)

The same [*Roll a Byte*](https://heap.45gfg9.net/rants/809bc23b9ca6/), right in your hands this time.

This is a device that generates a random byte and shows it on a 72x40 OLED display and 8 LEDs, powered by a CH32V003 ([v3](src/ch32v/)) or an ATmega88PA ([v2](src/avr/)), and the amazing [U8g2](https://github.com/olikraus/u8g2) library.

## Build

This repository is a PlatformIO project. To build the project, clone this repository and open in PlatformIO IDE / build with PlatformIO CLI.

```bash
git clone --recurse-submodules https://github.com/45gfg9/RollAByte.git
cd RollAByte
pio run -e v3
```

## Hardware

The schematic, PCB layout and EasyEDA source are available in the `hardware` directory.

![Schematic](hardware/RollAByte_v2_schematic.svg)

![PCB front](hardware/RollAByte_v2_PCB_front.svg)

![PCB back](hardware/RollAByte_v2_PCB_back.svg)

## License

This project is licensed under the WTFPL license. See the [LICENSE](LICENSE) file for details.

The font used in the display is [fcambus/spleen](https://github.com/fcambus/spleen), which is licensed under the BSD 2-Clause license.
