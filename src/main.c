/* Copyright © 2024 45gfg9 <45gfg9@45gfg9.net>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the LICENSE file for more details.
 */

#include <stdlib.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <u8x8_avr.h>

#ifndef ADC_PRESCALE_BITS
#if F_CPU >= 12800000L
#define ADC_PRESCALE_BITS (_BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0))
#elif F_CPU >= 6400000L
#define ADC_PRESCALE_BITS (_BV(ADPS2) | _BV(ADPS1))
#elif F_CPU >= 3200000L
#define ADC_PRESCALE_BITS (_BV(ADPS2) | _BV(ADPS0))
#elif F_CPU >= 1600000L
#define ADC_PRESCALE_BITS (_BV(ADPS2))
#elif F_CPU >= 800000L
#define ADC_PRESCALE_BITS (_BV(ADPS1) | _BV(ADPS0))
#else
#error "Define ADC_PRESCALE_BITS"
#endif
#endif

extern const uint8_t rab_font[] U8X8_FONT_SECTION("rab_font");

u8g2_t u8g2;

uint16_t adc_seedrand(void);
void u8g2_display_number(u8g2_t *u8g2, uint8_t val);
uint8_t u8x8_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

int main() {
  // shut down all peripherals except ADC
  PRR = (__AVR_HAVE_PRR & ~_BV(PRADC));

  // AVCC with external capacitor at AREF pin
  // use ADC6
  ADMUX = _BV(REFS0) | 6;

  srand(adc_seedrand());

  // shut down all peripherals except SPI
  PRR = (__AVR_HAVE_PRR & ~_BV(PRSPI));

  // SPI clock rate F_CPU/2
  SPSR = _BV(SPI2X);

  u8g2_Setup_ssd1306_72x40_er_f(&u8g2, &u8g2_cb_r0, u8x8_byte_avr_hw_spi, u8x8_gpio_and_delay);
  u8g2_InitDisplay(&u8g2);
  u8g2_ClearDisplay(&u8g2);
  u8g2_SetPowerSave(&u8g2, 0);
  u8g2_SetFont(&u8g2, rab_font);

  uint8_t rnd = rand();
  u8g2_display_number(&u8g2, rnd);
  u8g2_SendBuffer(&u8g2);

  SPCR = 0; // disable SPI

  PRR = __AVR_HAVE_PRR; // shut down all peripherals

  DDRD = 0xff;
  PORTD = rnd;
  // enable pull-ups on unused pins
  DDRB = 0;
  PORTB = 0xff;
  DDRC = 0;
  PORTC = 0xff;

  SMCR = _BV(SM1) | _BV(SE); // enable sleep mode (power down)
  while (1) {
    sleep_cpu();
  }
}

uint16_t adc_seedrand(void) {
  // repeat 17 times, take LSB of each conversion, and discard the first one
  uint16_t r = 0;

  for (uint8_t i = CHAR_BIT * sizeof r + 1; i; i--) {
    // start and wait for conversion to complete
    ADCSRA = _BV(ADEN) | _BV(ADSC) | ADC_PRESCALE_BITS;
    loop_until_bit_is_clear(ADCSRA, ADSC);
    r = (r << 1) | (ADC & 1);
    _delay_us(600);
  }
  ADCSRA = 0; // disable ADC

  return r;
}

void u8g2_display_number(u8g2_t *u8g2, uint8_t val) {
  char hex[9] = "0x";

  hex[2] = val >> 4;
  if (hex[2] > 9)
    hex[2] += 'a' - 10;
  else
    hex[2] += '0';

  hex[3] = val & 0xf;
  if (hex[3] > 9)
    hex[3] += 'a' - 10;
  else
    hex[3] += '0';

  hex[4] = 0;

  u8g2_DrawStr(u8g2, 4, 30, hex);
}

#define CS_DDR DDRB
#define CS_PORT PORTB
#define CS_BIT 2

#define DC_DDR DDRB
#define DC_PORT PORTB
#define DC_BIT 1

#define RESET_DDR DDRB
#define RESET_PORT PORTB
#define RESET_BIT 0

uint8_t u8x8_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
  // Re-use library for delays
  if (u8x8_avr_delay(u8x8, msg, arg_int, arg_ptr))
    return 1;

  switch (msg) {
    // called once during init phase of u8g2/u8x8
    // can be used to setup pins
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
      CS_DDR |= _BV(CS_BIT);
      DC_DDR |= _BV(DC_BIT);
      RESET_DDR |= _BV(RESET_BIT);
      break;
    // CS (chip select) pin: Output level in arg_int
    case U8X8_MSG_GPIO_CS:
      if (arg_int)
        CS_PORT |= _BV(CS_BIT);
      else
        CS_PORT &= ~_BV(CS_BIT);
      break;
    // DC (data/cmd, A0, register select) pin: Output level in arg_int
    case U8X8_MSG_GPIO_DC:
      if (arg_int)
        DC_PORT |= _BV(DC_BIT);
      else
        DC_PORT &= ~_BV(DC_BIT);
      break;
    // Reset pin: Output level in arg_int
    case U8X8_MSG_GPIO_RESET:
      if (arg_int)
        RESET_PORT |= _BV(RESET_BIT);
      else
        RESET_PORT &= ~_BV(RESET_BIT);
      break;
    default:
      u8x8_SetGPIOResult(u8x8, 1);
      break;
  }
  return 1;
}
