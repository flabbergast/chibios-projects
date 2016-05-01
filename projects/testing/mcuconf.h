/*
    ChibiOS/RT - Copyright (C) 2006-2014 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef _MCUCONF_H_
#define _MCUCONF_H_

#if defined(TEENSY30) || defined(TEENSY32) || defined(WF)

#define K20x_MCUCONF

/*
 * HAL driver system settings.
 */
/* PEE mode - 48MHz system clock driven by external crystal. */
#define KINETIS_MCG_MODE            KINETIS_MCG_MODE_PEE
#define KINETIS_PLLCLK_FREQUENCY    96000000UL
#define KINETIS_SYSCLK_FREQUENCY    48000000UL

#define KINETIS_USB_USE_USB0                    TRUE
#define KINETIS_EXTI_NUM_CHANNELS               2
#define KINETIS_EXT_PORTA_IRQ_PRIORITY          12
#define KINETIS_EXT_PORTB_IRQ_PRIORITY          12
#define KINETIS_EXT_PORTC_IRQ_PRIORITY          12
#define KINETIS_EXT_PORTD_IRQ_PRIORITY          12
#define KINETIS_EXT_PORTE_IRQ_PRIORITY          12
#define KINETIS_EXT_PORTA_WIDTH                 20
#define KINETIS_EXT_PORTB_WIDTH                 20
#define KINETIS_EXT_PORTC_WIDTH                 12
#define KINETIS_EXT_PORTD_WIDTH                 8
#define KINETIS_EXT_PORTE_WIDTH                 2

#define KINETIS_I2C_USE_I2C0                  TRUE
#define KINETIS_I2C_I2C0_PRIORITY             8

#define KINETIS_SERIAL_USE_UART0              TRUE

#endif /* TEENSY */


#if defined(MCHCK)

#define K20x_MCUCONF

/*
 * HAL driver system settings.
 */
/* FEI mode - 48 MHz with internal 32.768 kHz crystal */
#define KINETIS_MCG_MODE            KINETIS_MCG_MODE_FEI
#define KINETIS_MCG_FLL_DMX32       1           /* Fine-tune for 32.768 kHz */
#define KINETIS_MCG_FLL_DRS         1           /* 1464x FLL factor */
#define KINETIS_SYSCLK_FREQUENCY    47972352UL  /* 32.768 kHz * 1464 (~48 MHz) */
#define KINETIS_CLKDIV1_OUTDIV1     1
#define KINETIS_CLKDIV1_OUTDIV2     1
#define KINETIS_CLKDIV1_OUTDIV4     2
#define KINETIS_BUSCLK_FREQUENCY    KINETIS_SYSCLK_FREQUENCY
#define KINETIS_FLASHCLK_FREQUENCY  KINETIS_SYSCLK_FREQUENCY/2

#define KINETIS_USB_USE_USB0                    TRUE
#define KINETIS_EXTI_NUM_CHANNELS               2
#define KINETIS_EXT_PORTA_IRQ_PRIORITY          12
#define KINETIS_EXT_PORTB_IRQ_PRIORITY          12
#define KINETIS_EXT_PORTC_IRQ_PRIORITY          12
#define KINETIS_EXT_PORTD_IRQ_PRIORITY          12
#define KINETIS_EXT_PORTE_IRQ_PRIORITY          12
#define KINETIS_EXT_PORTA_WIDTH                 20
#define KINETIS_EXT_PORTB_WIDTH                 18
#define KINETIS_EXT_PORTC_WIDTH                 8
#define KINETIS_EXT_PORTD_WIDTH                 8
#define KINETIS_EXT_PORTE_WIDTH                 0

#define KINETIS_I2C_USE_I2C0                  TRUE
#define KINETIS_I2C_I2C0_PRIORITY             8

#define KINETIS_SERIAL_USE_UART0              TRUE

#endif /* MCHCK */


#if defined(KL27Z)

#define KL2x_MCUCONF

/*
 * HAL driver system settings.
 */
#if 1
/* High-frequency internal RC, 48MHz, possible USB clock recovery */
#define KINETIS_MCGLITE_MODE        KINETIS_MCGLITE_MODE_HIRC
#define KINETIS_SYSCLK_FREQUENCY    48000000UL
#define KINETIS_CLKDIV1_OUTDIV1     1
#endif

#if 0
/* Low-frequency internal RC, 8 MHz mode */
#define KINETIS_MCGLITE_MODE        KINETIS_MCGLITE_MODE_LIRC8M
#define KINETIS_SYSCLK_FREQUENCY    8000000UL
#define KINETIS_CLKDIV1_OUTDIV1     1
#endif

/*
 * USB driver settings
 */
#define KINETIS_USB_USE_USB0                  TRUE
/* need to redefine this, since the default is for K20x */
#define KINETIS_USB_USB0_IRQ_PRIORITY         2

/*
 * I2C driver settings
 */
#define KINETIS_I2C_USE_I2C0                  TRUE
#define KINETIS_I2C_I2C0_PRIORITY             2

/*
 * Serial driver settings
 */
#define KINETIS_SERIAL_USE_UART0              TRUE

/*
 * EXT driver settings
 */
#define KINETIS_EXTI_NUM_CHANNELS               2
#define KINETIS_EXT_PORTA_IRQ_PRIORITY          2
#define KINETIS_EXT_PORTD_IRQ_PRIORITY          2
#define KINETIS_EXT_PORTA_WIDTH                 21
#define KINETIS_EXT_PORTB_WIDTH                 0
#define KINETIS_EXT_PORTC_WIDTH                 4
#define KINETIS_EXT_PORTD_WIDTH                 8
#define KINETIS_EXT_PORTE_WIDTH                 0

/*
 * Kinetis FOPT configuration byte
 */
/* for KL27: */
#define KINETIS_NV_FOPT_BYTE 0x3D
/* NV_FOPT: bit7-6/BOOTSRC_SEL=0b00 (11=from ROM; 00=from FLASH)
   bit5/FAST_INIT=1, bit4/LPBOOT1=1,
   bit3/RESET_PIN_CFG=1, bit2/NMI_DIS=1,
   bit1/BOOTPIN_OPT=0, bit0/LPBOOT0=1 */
/* BOOTPIN_OPT: 1=boot depends on BOOTSRC_SEL
   0=boot samples BOOTCFG0=NMI pin */
/* Boot sequence, page 88 of manual:
 * - If the NMI/BOOTCFG0 input is high or the NMI function is disabled in FTFA_FOPT, the CPU begins execution at the PC location.
 * - If the NMI/BOOTCFG0 input is low, the NMI function is enabled in FTFA_FOPT, and FTFA_FOPT[BOOTPIN_OPT] = 1, this results in an NMI interrupt. The processor executes an Exception Entry and reads the NMI interrupt handler address from vector-table offset 8. The CPU begins execution at the NMI interrupt handler.
 * - When FTFA_FOPT[BOOTPIN_OPT] = 0, it forces boot from ROM if NMI/BOOTCFG0 pin set to 0.
 *
 * Observed behaviour:
 * - when BOOTPIN_OPT=0, BOOTSRC_SEL still matters:
 *   - if 0b11 (from ROM), it still boots from ROM, even if BOOTCFG0 pin
 *     is high/floating, but leaves ROM and runs user app after
 *     5 seconds delay.
 *   - if 0b00 (from FLASH), reset/powerup jumps to user app unless
 *     BOOTCFG0 pin is asserted.
 * - in any case, reset when in bootloader induces the 5 second delay
 *   before starting the user app.
 * 
 */

#endif /* KL27Z */

#if defined(TEENSYLC)

#define KL2x_MCUCONF

/*
 * HAL driver system settings.
 */
#if 1
/* PEE mode - 48MHz system clock driven by (8 MHz) external crystal. */
#define KINETIS_MCG_MODE            KINETIS_MCG_MODE_PEE
#define KINETIS_PLLCLK_FREQUENCY    96000000UL
#define KINETIS_SYSCLK_FREQUENCY    48000000UL
#endif

#if 0
/* crystal-less FEI mode - 48 MHz with internal 32.768 kHz crystal */
#define KINETIS_MCG_MODE            KINETIS_MCG_MODE_FEI
#define KINETIS_MCG_FLL_DMX32       1           /* Fine-tune for 32.768 kHz */
#define KINETIS_MCG_FLL_DRS         1           /* 1464x FLL factor */
#define KINETIS_SYSCLK_FREQUENCY    47972352UL  /* 32.768 kHz * 1464 (~48 MHz) */
#define KINETIS_CLKDIV1_OUTDIV1     1           /* do not divide system clock */
#endif

/*
 * SERIAL driver system settings.
 */
#define KINETIS_SERIAL_USE_UART0              TRUE

/*
 * I2C driver settings
 */
#define KINETIS_I2C_USE_I2C0                  TRUE
#define KINETIS_I2C_I2C0_PRIORITY             2

/*
 * USB driver settings
 */
#define KINETIS_USB_USE_USB0                  TRUE
/* need to redefine this, since the default is for K20x */
#define KINETIS_USB_USB0_IRQ_PRIORITY         2

/*
 * EXTI driver system settings.
 */
#define KINETIS_EXTI_NUM_CHANNELS               1
#define KINETIS_EXT_PORTA_IRQ_PRIORITY          3
#define KINETIS_EXT_PORTB_IRQ_PRIORITY          3
#define KINETIS_EXT_PORTC_IRQ_PRIORITY          3
#define KINETIS_EXT_PORTD_IRQ_PRIORITY          3
#define KINETIS_EXT_PORTE_IRQ_PRIORITY          3

/* KL26 48pin  */
#define KINETIS_EXT_PORTA_WIDTH                 21
#define KINETIS_EXT_PORTB_WIDTH                 18
#define KINETIS_EXT_PORTC_WIDTH                 8
#define KINETIS_EXT_PORTD_WIDTH                 8
#define KINETIS_EXT_PORTE_WIDTH                 31

#endif /* TEENSYLC */


#if defined(KL25Z)

#define KL2x_MCUCONF

/* FEI mode - ~24MHz */
#if 0
#define KINETIS_MCG_MODE            KINETIS_MCG_MODE_FEI
#define KINETIS_MCG_FLL_DMX32       1           /* Fine-tune for 32.768 kHz */
#define KINETIS_MCG_FLL_DRS         0           /* 732x FLL factor */
#define KINETIS_SYSCLK_FREQUENCY    23986176UL  /* 32.768 kHz * 732 (~24 MHz) */
#define KINETIS_CLKDIV1_OUTDIV1     1           /* Divide MCGCLKOUT (~24MHz) by 1 to SYSCLK */
#define KINETIS_CLKDIV1_OUTDIV4     2           /* Divide by 2 for (~12MHz) bus/flash clock */
#endif /* 0 */

/* FEE mode - 24 MHz with external 32.768 kHz crystal */
#if 0
#define KINETIS_MCG_MODE            KINETIS_MCG_MODE_FEE
#define KINETIS_MCG_FLL_DMX32       1           /* Fine-tune for 32.768 kHz */
#define KINETIS_MCG_FLL_DRS         0           /* 732x FLL factor */
#define KINETIS_CLKDIV1_OUTDIV1     1           /* Divide 48 MHz FLL by 1 => 24 MHz */
#define KINETIS_CLKDIV1_OUTDIV4     2           /* Divide OUTDIV1 output by 2 => 12 MHz */
#define KINETIS_SYSCLK_FREQUENCY    23986176UL  /* 32.768 kHz*732 (~24 MHz) */
#define KINETIS_UART0_CLOCK_FREQ    (32768 * 732) /* FLL output */
#define KINETIS_UART0_CLOCK_SRC     1           /* Select FLL clock */
#endif /* 0 */

/* FEE mode - 48 MHz */
#if 0
#define KINETIS_MCG_MODE            KINETIS_MCG_MODE_FEE
#define KINETIS_MCG_FLL_DMX32       1           /* Fine-tune for 32.768 kHz */
#define KINETIS_MCG_FLL_DRS         1           /* 1464x FLL factor */
#define KINETIS_CLKDIV1_OUTDIV1     1           /* Divide 48 MHz FLL by 1 => 48 MHz */
#define KINETIS_CLKDIV1_OUTDIV4     2           /* Divide OUTDIV1 output by 2 => 24 MHz */
#define KINETIS_SYSCLK_FREQUENCY    47972352UL  /* 32.768 kHz * 1464 (~48 MHz) */
#endif /* 0 */


/*
 * USB driver settings
 */
#define KINETIS_USB_USE_USB0                  TRUE
/* need to redefine this, since the default is for K20x */
#define KINETIS_USB_USB0_IRQ_PRIORITY         2

/*
 * I2C driver settings
 */
#define KINETIS_I2C_USE_I2C0                  TRUE
#define KINETIS_I2C_I2C0_PRIORITY             2

/*
 * SERIAL driver system settings.
 */
#define KINETIS_SERIAL_USE_UART0              TRUE

/*
 * EXTI driver system settings.
 */
#define KINETIS_EXTI_NUM_CHANNELS               1
#define KINETIS_EXT_PORTA_IRQ_PRIORITY          3
#define KINETIS_EXT_PORTB_IRQ_PRIORITY          3
#define KINETIS_EXT_PORTC_IRQ_PRIORITY          3
#define KINETIS_EXT_PORTD_IRQ_PRIORITY          3
#define KINETIS_EXT_PORTE_IRQ_PRIORITY          3

/* KL25 80pin  */
#define KINETIS_EXT_PORTA_WIDTH                 21
#define KINETIS_EXT_PORTD_WIDTH                 8

#endif /* KL25Z */

/* fill in Kinetis port widths if not set */
#ifndef KINETIS_EXT_PORTA_WIDTH
#define KINETIS_EXT_PORTA_WIDTH                 0
#endif
#ifndef KINETIS_EXT_PORTB_WIDTH
#define KINETIS_EXT_PORTB_WIDTH                 0
#endif
#ifndef KINETIS_EXT_PORTC_WIDTH
#define KINETIS_EXT_PORTC_WIDTH                 0
#endif
#ifndef KINETIS_EXT_PORTD_WIDTH
#define KINETIS_EXT_PORTD_WIDTH                 0
#endif
#ifndef KINETIS_EXT_PORTE_WIDTH
#define KINETIS_EXT_PORTE_WIDTH                 0
#endif



#if defined(F042)

/*
 * STM32F0xx drivers configuration.
 * The following settings override the default settings present in
 * the various device driver implementation headers.
 * Note that the settings for each driver only have effect if the whole
 * driver is enabled in halconf.h.
 *
 * IRQ priorities:
 * 3...0       Lowest...Highest.
 *
 * DMA priorities:
 * 0...3        Lowest...Highest.
 */

#define STM32F0xx_MCUCONF

/*
 * HAL driver system settings.
 */
#define STM32_NO_INIT                       FALSE
#define STM32_PVD_ENABLE                    FALSE
#define STM32_PLS                           STM32_PLS_LEV0
#define STM32_HSI_ENABLED                   TRUE
#define STM32_HSI14_ENABLED                 TRUE
#define STM32_HSI48_ENABLED                 FALSE
#define STM32_LSI_ENABLED                   TRUE
#define STM32_HSE_ENABLED                   FALSE
#define STM32_LSE_ENABLED                   FALSE
#define STM32_SW                            STM32_SW_PLL
#define STM32_PLLSRC                        STM32_PLLSRC_HSI_DIV2
#define STM32_PREDIV_VALUE                  1
#define STM32_PLLMUL_VALUE                  12
#define STM32_HPRE                          STM32_HPRE_DIV1
#define STM32_PPRE                          STM32_PPRE_DIV1
#define STM32_ADCSW                         STM32_ADCSW_HSI14
#define STM32_ADCPRE                        STM32_ADCPRE_DIV4
#define STM32_MCOSEL                        STM32_MCOSEL_NOCLOCK
#define STM32_ADCPRE                        STM32_ADCPRE_DIV4
#define STM32_ADCSW                         STM32_ADCSW_HSI14
#define STM32_USBSW                         STM32_USBSW_HSI48
#define STM32_CECSW                         STM32_CECSW_HSI
#define STM32_I2C1SW                        STM32_I2C1SW_HSI
#define STM32_USART1SW                      STM32_USART1SW_PCLK
#define STM32_RTCSEL                        STM32_RTCSEL_LSI

/*
 * ADC driver system settings.
 */
#define STM32_ADC_USE_ADC1                  FALSE
#define STM32_ADC_ADC1_DMA_PRIORITY         2
#define STM32_ADC_IRQ_PRIORITY              2
#define STM32_ADC_ADC1_DMA_IRQ_PRIORITY     2

/*
 * EXT driver system settings.
 */
#define STM32_EXT_EXTI0_1_IRQ_PRIORITY      3
#define STM32_EXT_EXTI2_3_IRQ_PRIORITY      3
#define STM32_EXT_EXTI4_15_IRQ_PRIORITY     3
#define STM32_EXT_EXTI16_IRQ_PRIORITY       3
#define STM32_EXT_EXTI17_IRQ_PRIORITY       3

/*
 * GPT driver system settings.
 */
#define STM32_GPT_USE_TIM1                  FALSE
#define STM32_GPT_USE_TIM2                  FALSE
#define STM32_GPT_USE_TIM3                  FALSE
#define STM32_GPT_USE_TIM14                 FALSE
#define STM32_GPT_TIM1_IRQ_PRIORITY         2
#define STM32_GPT_TIM2_IRQ_PRIORITY         2
#define STM32_GPT_TIM3_IRQ_PRIORITY         2
#define STM32_GPT_TIM14_IRQ_PRIORITY        2

/*
 * I2C driver system settings.
 */
#define STM32_I2C_USE_I2C1                  FALSE
#define STM32_I2C_USE_I2C2                  FALSE
#define STM32_I2C_BUSY_TIMEOUT              50
#define STM32_I2C_I2C1_IRQ_PRIORITY         3
#define STM32_I2C_I2C2_IRQ_PRIORITY         3
#define STM32_I2C_USE_DMA                   TRUE
#define STM32_I2C_I2C1_DMA_PRIORITY         1
#define STM32_I2C_I2C2_DMA_PRIORITY         1
#define STM32_I2C_DMA_ERROR_HOOK(i2cp)      osalSysHalt("DMA failure")

/*
 * ICU driver system settings.
 */
#define STM32_ICU_USE_TIM1                  FALSE
#define STM32_ICU_USE_TIM2                  FALSE
#define STM32_ICU_USE_TIM3                  FALSE
#define STM32_ICU_TIM1_IRQ_PRIORITY         3
#define STM32_ICU_TIM2_IRQ_PRIORITY         3
#define STM32_ICU_TIM3_IRQ_PRIORITY         3

/*
 * PWM driver system settings.
 */
#define STM32_PWM_USE_ADVANCED              FALSE
#define STM32_PWM_USE_TIM1                  FALSE
#define STM32_PWM_USE_TIM2                  FALSE
#define STM32_PWM_USE_TIM3                  FALSE
#define STM32_PWM_TIM1_IRQ_PRIORITY         3
#define STM32_PWM_TIM2_IRQ_PRIORITY         3
#define STM32_PWM_TIM3_IRQ_PRIORITY         3

/*
 * SERIAL driver system settings.
 */
#define STM32_SERIAL_USE_USART1             FALSE
#define STM32_SERIAL_USE_USART2             FALSE
#define STM32_SERIAL_USART1_PRIORITY        3
#define STM32_SERIAL_USART2_PRIORITY        3

/*
 * SPI driver system settings.
 */
#define STM32_SPI_USE_SPI1                  FALSE
#define STM32_SPI_USE_SPI2                  FALSE
#define STM32_SPI_SPI1_DMA_PRIORITY         1
#define STM32_SPI_SPI2_DMA_PRIORITY         1
#define STM32_SPI_SPI1_IRQ_PRIORITY         2
#define STM32_SPI_SPI2_IRQ_PRIORITY         2
#define STM32_SPI_DMA_ERROR_HOOK(spip)      osalSysHalt("DMA failure")

/*
 * ST driver system settings.
 */
#define STM32_ST_IRQ_PRIORITY               2
#define STM32_ST_USE_TIMER                  2

/*
 * UART driver system settings.
 */
#define STM32_UART_USE_USART1               FALSE
#define STM32_UART_USE_USART2               FALSE
#define STM32_UART_USART1_IRQ_PRIORITY      3
#define STM32_UART_USART2_IRQ_PRIORITY      3
#define STM32_UART_USART1_DMA_PRIORITY      0
#define STM32_UART_USART2_DMA_PRIORITY      0
#define STM32_UART_DMA_ERROR_HOOK(uartp)    osalSysHalt("DMA failure")

/*
 * USB driver system settings.
 */
#define STM32_USB_USE_USB1                  TRUE
#define STM32_USB_LOW_POWER_ON_SUSPEND      FALSE
#define STM32_USB_USB1_LP_IRQ_PRIORITY      3

#endif /* F042 */

#endif /* _MCUCONF_H_ */