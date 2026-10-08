The fuse settings are set to use the internal 8MHz oscillator with the CKDIV8 fuse disabled. For ATmega88PA, the fuses are set as follows:

- Low: `0xe2`
- High: `0xdf`
- Extended: `0xff`

```bash
avrdude -pm88p -cusbasp -Ulfuse:w:0xe2:m -Uhfuse:w:0xdf:m -Uefuse:w:0xff:m
```
