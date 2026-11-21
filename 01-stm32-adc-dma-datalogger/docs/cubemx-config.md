# CubeMX configuration (STM32G474)

Values below are what the firmware in `firmware/App` expects.

## Clocks
| Item | Value |
|---|---|
| HSE | 8 MHz crystal, or HSI16 if no crystal |
| SYSCLK | 170 MHz (PLL, boost mode on) |
| ADC clock | Synchronous HCLK / 4 (42.5 MHz) |

## ADC1
| Setting | Value |
|---|---|
| Channels | IN1 (PA0), IN2 (PA1), IN3 (PA2), IN4 (PA3) |
| Resolution | 12 bit, right aligned |
| Scan conversion mode | Enabled, 4 ranks (IN1, IN2, IN3, IN4) |
| Continuous conversion | **Disabled** |
| DMA continuous requests | **Enabled** |
| External trigger | **Timer 6 Trigger Out event**, rising edge |
| Sampling time | 47.5 cycles (suits sources up to about 10 kohm) |
| End of conversion | End of sequence |
| Overrun | Data preserved |

## DMA (for ADC1)
| Setting | Value |
|---|---|
| Mode | **Circular** |
| Data width | Half word (peripheral and memory) |
| Memory increment | Enabled |

## TIM6 (sample clock)
| Setting | Value |
|---|---|
| Prescaler | 169 (gives 1 MHz) |
| Counter period (ARR) | 99 (gives 10 kHz scan rate) |
| Trigger output (TRGO) | **Update event** |

Changing ARR changes the per-channel sample rate: `fs = 1 MHz / (ARR + 1)`.

## USART2
| Setting | Value |
|---|---|
| Mode | Asynchronous |
| Baud | 115200, 8N1 |
| Pins | PA2/PA3 are used by the ADC here, so remap USART2 to PB3 (TX) / PB4 (RX) |

## NVIC
Enable the DMA channel interrupt for ADC1. ADC interrupts are not needed.

> Check pin choices against the datasheet alternate-function table for your exact package
> before wiring. Pin availability differs between STM32G474 packages, and the ADC trigger
> mapping should be confirmed in CubeMX (it lists the valid trigger sources for ADC1).
