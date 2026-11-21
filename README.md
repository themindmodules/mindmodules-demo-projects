# MindModules Demo Projects

Five complete example projects from [@themindmodules](https://instagram.com/themindmodules): PCB design,
STM32, Arduino, core electronics, a hobby build and a hackathon kit. Each folder is a self-contained project
with its own README, documentation, design files and BOM.

| # | Project | Type | What is inside |
|---|---|---|---|
| 01 | [stm32-adc-dma-datalogger](01-stm32-adc-dma-datalogger) | STM32 firmware | 4-channel ADC + DMA logger, CubeMX settings, timing maths, test plan |
| 02 | [arduino-plant-monitor](02-arduino-plant-monitor) | Arduino | Soil moisture + safe auto-watering, wiring, calibration, safety notes |
| 03 | [lm317-adjustable-supply](03-lm317-adjustable-supply) | **PCB** | KiCad schematic + PCB, gerbers, BOM, assembly and ordering guide |
| 04 | [nano-rgb-led-controller](04-nano-rgb-led-controller) | Hobby / mixed | 12 V RGB strip controller: schematic, perfboard guide, firmware |
| 05 | [hackathon-gridwatch](05-hackathon-gridwatch) | Hackathon | Energy anomaly detection demo, pitch, judge Q&A, 24 h plan |

## Status

These are reference designs. The KiCad files pass ERC/DRC and the code compiles, but **no board here has been
fabricated or bench-tested**, and the hackathon demo runs on simulated data. Every README says exactly what was
and was not verified. Follow the test plans before trusting any design, and take care with anything that touches mains voltage.

## Want more?

Exclusive PCB designs, STM32 and Arduino projects, documentation, personal guidance, Q&A and doubt calls are
available to Inner Circle subscribers on Instagram: [@themindmodules](https://instagram.com/themindmodules).

## License

See `LICENSE.md` inside each project.
