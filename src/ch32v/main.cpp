#include <ch32v00x.h>
#include <stdlib.h>
#include <debug.h>

#include "U8g2lib.h"

#define BA(x) (static_cast<BitAction>(!!(x)))

extern const uint8_t rab_font[] U8X8_FONT_SECTION("rab_font");

// CH32V003 SPI1 default pins: SCK=PC5, MOSI=PC6
// All OLED pins are GPIOC
static constexpr uint16_t OLED_PIN_CS = GPIO_Pin_1;
static constexpr uint16_t OLED_PIN_DC = GPIO_Pin_3;
static constexpr uint16_t OLED_PIN_RST = GPIO_Pin_2;

// A2 = PC4 floating pin
static constexpr uint16_t ADC_PIN = GPIO_Pin_4;
static const auto ADC_PORT = GPIOC;
static constexpr auto ADC_CHANNEL = ADC_Channel_2;

static constexpr uint16_t POWER_PIN = GPIO_Pin_0;
static const auto POWER_PORT = GPIOD;

// AWU wake period is approximate and depends on LSI frequency tolerance.
static constexpr uint32_t AWU_TARGET_WAKE_MS = 1500;
static constexpr uint32_t AWU_LSI_HZ = 128000;

static U8G2 u8g2;

static void gpio_init() {
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);

  GPIO_InitTypeDef gpio {};

  gpio.GPIO_Pin = GPIO_Pin_All;
  gpio.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_Init(GPIOA, &gpio);
  GPIO_Init(GPIOC, &gpio);
  GPIO_Init(GPIOD, &gpio);

  gpio.GPIO_Pin = POWER_PIN;
  gpio.GPIO_Speed = GPIO_Speed_30MHz;
  gpio.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(POWER_PORT, &gpio);

  GPIO_WriteBit(POWER_PORT, POWER_PIN, Bit_SET);
}

static void power_pin_set_low() {
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
  GPIO_WriteBit(POWER_PORT, POWER_PIN, Bit_RESET);
}

static void oled_init_ctrl_pins() {
  GPIO_InitTypeDef gpio {};
  gpio.GPIO_Pin = OLED_PIN_CS | OLED_PIN_DC | OLED_PIN_RST;
  gpio.GPIO_Speed = GPIO_Speed_30MHz;
  gpio.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC, &gpio);

  // SSD1306 idle: CS high, DC high, RESET high.
  GPIO_SetBits(GPIOC, OLED_PIN_CS | OLED_PIN_DC | OLED_PIN_RST);
}

static void oled_init_spi1() {
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

  GPIO_InitTypeDef gpio {};

  // SCK(PC5), MOSI(PC6) as AF push-pull.
  gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6;
  gpio.GPIO_Speed = GPIO_Speed_30MHz;
  gpio.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOC, &gpio);

  SPI_InitTypeDef spi {};
  SPI_StructInit(&spi);
  spi.SPI_Direction = SPI_Direction_1Line_Tx;
  spi.SPI_Mode = SPI_Mode_Master;
  spi.SPI_DataSize = SPI_DataSize_8b;
  spi.SPI_CPOL = SPI_CPOL_Low;
  spi.SPI_CPHA = SPI_CPHA_1Edge;
  spi.SPI_NSS = SPI_NSS_Soft;
  spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
  spi.SPI_FirstBit = SPI_FirstBit_MSB;
  spi.SPI_CRCPolynomial = 7;
  SPI_Init(SPI1, &spi);
  SPI_Cmd(SPI1, ENABLE);
}

static inline void spi_send_byte(uint8_t data) {
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)
    ;
  SPI_I2S_SendData(SPI1, data);
}

static uint8_t u8x8_byte_ch32_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
  uint8_t *data;

  switch (msg) {
    case U8X8_MSG_BYTE_INIT:
      oled_init_spi1();
      break;

    case U8X8_MSG_BYTE_SEND:
      data = static_cast<uint8_t *>(arg_ptr);
      while (arg_int > 0) {
        spi_send_byte(*data++);
        arg_int--;
      }
      break;

    case U8X8_MSG_BYTE_SET_DC:
      GPIO_WriteBit(GPIOC, OLED_PIN_DC, BA(arg_int));
      break;

    case U8X8_MSG_BYTE_START_TRANSFER:
      GPIO_WriteBit(GPIOC, OLED_PIN_CS, BA(u8x8->display_info->chip_enable_level));
      u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns);
      break;

    case U8X8_MSG_BYTE_END_TRANSFER:
      while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) != RESET)
        ;
      u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns);
      GPIO_WriteBit(GPIOC, OLED_PIN_CS, BA(u8x8->display_info->chip_disable_level));
      break;

    default:
      break;
  }

  return 1;
}

static uint8_t u8x8_gpio_and_delay_ch32(u8x8_t * /*u8x8*/, uint8_t msg, uint8_t arg_int, void * /*arg_ptr*/) {
  switch (msg) {
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
      oled_init_ctrl_pins();
      break;

    case U8X8_MSG_DELAY_MILLI:
      Delay_Ms(arg_int);
      break;

    case U8X8_MSG_DELAY_10MICRO:
      Delay_Us(10 * arg_int);
      break;

    case U8X8_MSG_GPIO_CS:
      GPIO_WriteBit(GPIOC, OLED_PIN_CS, BA(arg_int));
      break;

    case U8X8_MSG_GPIO_DC:
      GPIO_WriteBit(GPIOC, OLED_PIN_DC, BA(arg_int));
      break;

    case U8X8_MSG_GPIO_RESET:
      GPIO_WriteBit(GPIOC, OLED_PIN_RST, BA(arg_int));
      break;

    default:
      break;
  }

  return 1;
}

static void adc_init() {
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_ADC1, ENABLE);
  RCC_ADCCLKConfig(RCC_PCLK2_Div8);

  GPIO_InitTypeDef gpio {};
  gpio.GPIO_Pin = ADC_PIN;
  gpio.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(ADC_PORT, &gpio);

  ADC_InitTypeDef adc {};
  ADC_StructInit(&adc);
  adc.ADC_Mode = ADC_Mode_Independent;
  adc.ADC_ScanConvMode = DISABLE;
  adc.ADC_ContinuousConvMode = DISABLE;
  adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  adc.ADC_DataAlign = ADC_DataAlign_Right;
  adc.ADC_NbrOfChannel = 1;
  ADC_Init(ADC1, &adc);

  ADC_Cmd(ADC1, ENABLE);

  ADC_ResetCalibration(ADC1);
  while (ADC_GetResetCalibrationStatus(ADC1) == SET)
    ;

  ADC_StartCalibration(ADC1);
  while (ADC_GetCalibrationStatus(ADC1) == SET)
    ;
}

static uint16_t adc_read() {
  ADC_RegularChannelConfig(ADC1, ADC_CHANNEL, 1, ADC_SampleTime_241Cycles);
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);

  while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET)
    ;

  return ADC_GetConversionValue(ADC1);
}

// Introduces a variable delay based on the noisy LSBs of the
// previous ADC read. This prevents the ADC from sampling at
// a fixed frequency and syncing up with 50/60Hz mains hum.
static void jitter_delay(uint16_t noisy_val) {
  // Mask to the low bits so we don't delay too long
  volatile uint8_t delay_cycles = noisy_val & 0xff;

  while (delay_cycles--) {
    // Prevents compiler from optimizing away this empty loop.
    __NOP();
  }
}

static uint32_t rand_adc() {
  adc_init();

  uint32_t seed = 0;
  for (uint8_t i = 0; i < sizeof(seed) * 8; ++i) {
    uint16_t val = adc_read();
    seed = (seed << 1) | (val & 1);
    Delay_Us(10);
    jitter_delay(val);
  }

  ADC_Cmd(ADC1, DISABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);

  return seed;
}

static uint32_t splitmix32_next(uint32_t &state) {
  uint32_t z = (state += 0x6d2b79f5);
  z = (z ^ (z >> 15)) * (z | 1);
  z ^= z + (z ^ (z >> 7)) * (z | 61);
  return z ^ (z >> 14);
}

static void apply_random_bits_to_pins(uint8_t value) {
  GPIO_InitTypeDef gpio {};
  gpio.GPIO_Speed = GPIO_Speed_30MHz;
  gpio.GPIO_Mode = GPIO_Mode_Out_PP;

  gpio.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
  GPIO_Init(GPIOA, &gpio);

  gpio.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
  GPIO_Init(GPIOD, &gpio);

  gpio.GPIO_Pin = GPIO_Pin_7;
  GPIO_Init(GPIOC, &gpio);

  // bit7..0 -> {PA2, PA1, PD6, PD5, PD4, PD3, PD2, PC7}
  GPIO_WriteBit(GPIOA, GPIO_Pin_2, BA((value >> 7) & 1));
  GPIO_WriteBit(GPIOA, GPIO_Pin_1, BA((value >> 6) & 1));
  GPIO_WriteBit(GPIOD, GPIO_Pin_6, BA((value >> 5) & 1));
  GPIO_WriteBit(GPIOD, GPIO_Pin_5, BA((value >> 4) & 1));
  GPIO_WriteBit(GPIOD, GPIO_Pin_4, BA((value >> 3) & 1));
  GPIO_WriteBit(GPIOD, GPIO_Pin_3, BA((value >> 2) & 1));
  GPIO_WriteBit(GPIOD, GPIO_Pin_2, BA((value >> 1) & 1));
  GPIO_WriteBit(GPIOC, GPIO_Pin_7, BA(value & 1));
}

struct AwuSettings {
  uint32_t prescaler;
  uint8_t window;
};

static constexpr AwuSettings awu_for_target_ms(uint32_t target_ms) {
  const struct AwuOption {
    uint32_t reg_value;
    uint32_t divider;
  } options[] {
      {PWR_AWU_Prescaler_1, 1},       {PWR_AWU_Prescaler_2, 2},         {PWR_AWU_Prescaler_4, 4},
      {PWR_AWU_Prescaler_8, 8},       {PWR_AWU_Prescaler_16, 16},       {PWR_AWU_Prescaler_32, 32},
      {PWR_AWU_Prescaler_64, 64},     {PWR_AWU_Prescaler_128, 128},     {PWR_AWU_Prescaler_256, 256},
      {PWR_AWU_Prescaler_512, 512},   {PWR_AWU_Prescaler_1024, 1024},   {PWR_AWU_Prescaler_2048, 2048},
      {PWR_AWU_Prescaler_4096, 4096}, {PWR_AWU_Prescaler_10240, 10240}, {PWR_AWU_Prescaler_61440, 61440},
  };

  if (target_ms == 0) {
    target_ms = 1;
  }

  // Approximate AWU period with: T = window * prescaler / LSI.
  const uint64_t target_ticks = (static_cast<uint64_t>(target_ms) * AWU_LSI_HZ + 500ULL) / 1000ULL;

  uint64_t best_error = UINT64_MAX;
  uint32_t best_prescaler = PWR_AWU_Prescaler_61440;
  uint8_t best_window = 1;

  for (auto [reg_value, divider] : options) {
    uint64_t window = (target_ticks + (divider / 2ULL)) / divider;
    if (window < 1ULL) {
      window = 1ULL;
    }
    if (window > 63ULL) {
      window = 63ULL;
    }

    const uint64_t actual_ticks = window * divider;
    const uint64_t error =
        (actual_ticks > target_ticks) ? (actual_ticks - target_ticks) : (target_ticks - actual_ticks);

    if (error < best_error) {
      best_error = error;
      best_prescaler = reg_value;
      best_window = static_cast<uint8_t>(window);
    }
  }

  return {best_prescaler, best_window};
}

[[noreturn]] static void enter_powerdown_seq() {
  RCC_LSICmd(ENABLE);
  while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
    ;

  constexpr auto awu = awu_for_target_ms(AWU_TARGET_WAKE_MS);
  PWR_AWU_SetPrescaler(awu.prescaler);
  PWR_AWU_SetWindowValue(awu.window);
  PWR_AutoWakeUpCmd(ENABLE);

  SPI_Cmd(SPI1, DISABLE);

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD
                             | RCC_APB2Periph_ADC1 | RCC_APB2Periph_TIM1 | RCC_APB2Periph_SPI1 | RCC_APB2Periph_USART1,
                         DISABLE);

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | RCC_APB1Periph_I2C1 | RCC_APB1Periph_WWDG, DISABLE);

  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);

  EXTI_InitTypeDef exti {};
  exti.EXTI_Line = EXTI_Line9;
  exti.EXTI_Mode = EXTI_Mode_Event;
  exti.EXTI_Trigger = EXTI_Trigger_Rising;
  exti.EXTI_LineCmd = ENABLE;
  EXTI_Init(&exti);

  PWR_EnterSTANDBYMode(PWR_STANDBYEntry_WFE);

  EXTI_DeInit();
  power_pin_set_low();

  // Stop periodic wakeups and keep CPU in deepest practical idle after resume.
  PWR_AutoWakeUpCmd(DISABLE);
  RCC_LSICmd(DISABLE);

  while (1) {
    __WFI();
  }
}

int main() {
  gpio_init();

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  SystemCoreClockUpdate();
  Delay_Init();

  uint32_t state = rand_adc();
  // mix the state a bit more before using it
  splitmix32_next(state);
  splitmix32_next(state);

  u8g2_Setup_ssd1306_72x40_er_f(u8g2.getU8g2(), U8G2_R2, u8x8_byte_ch32_hw_spi, u8x8_gpio_and_delay_ch32);

  u8g2.initDisplay();
  u8g2.setPowerSave(0);
  u8g2.setFont(rab_font);
  uint8_t rnd = splitmix32_next(state) >> 24;
  char buf[5];
  snprintf(buf, sizeof(buf), "0x%02x", rnd);
  u8g2.drawStr(4, 30, buf);
  u8g2.sendBuffer();
  apply_random_bits_to_pins(rnd);

  enter_powerdown_seq();
}

extern "C" void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
extern "C" void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void NMI_Handler() {}
void HardFault_Handler() {
  while (1)
    ;
}
