# Design notes

These notes explain each part of one channel. Channel 2 is a copy of channel 1
(R3, R4, R6, Q2, D2, D4, K2, J2 instead of R1, R2, R5, Q1, D1, D3, K1, J1).

All numbers below are hand calculations from typical datasheet values. They were not simulated.

## Why a transistor at all

A microcontroller pin can give about 20 mA at best (an Arduino Uno) and often much less
(3.3 V boards are happier at 4 to 8 mA). The SRD-05VDC-SL-C coil needs about 71 mA. So the pin
only drives the base of a transistor, and the transistor carries the coil current from the 5 V supply.

## The coil

| Item | Value |
|---|---|
| Coil resistance | about 70 ohm (standard SRD 5 V coil) |
| Coil current | 5 V / 70 ohm = **71 mA** |
| Coil power | 5 V x 71 mA = 0.36 W |
| Must operate voltage | 3.75 V or less (75 percent of nominal) |

The coil connects between +5V (relay pin 2) and the transistor collector (relay pin 5).
This is a low side switch: the transistor sits between the coil and ground.

## Transistor choice: BC337

| Item | BC337 | Needed here |
|---|---|---|
| Collector current max | 800 mA | 71 mA |
| VCEO max | 45 V | about 6 V (see flyback below) |
| hFE at 100 mA | 100 minimum (BC337), 160 minimum for the -25 grade | |
| Power in TO-92 | 625 mW | about 15 mW |

The BC547 is often used for this job, but its 100 mA limit is close to the coil current.
The BC337 has plenty of margin and costs the same.

**Pinout warning.** The board follows the BC337 order: collector, base, emitter (pin 1 to pin 3,
flat face towards you, pins down). A 2N2222A or PN2222A in TO-92 is emitter, base, collector, so
it would go in back to front. If you swap the part, check its datasheet and rotate it.

## Base resistor: 470 ohm

We want the transistor fully on (saturated), so Vce is small and it stays cool.
A common rule is to give the base about one tenth of the collector current, which is a forced
gain of 10. Anything below the real minimum hFE (100) still saturates, just with less margin.

Base current is (Vin minus Vbe) / Rb, with Vbe about 0.8 V when saturated. The 10k pull-down
takes a little of it (0.8 V / 10k = 0.08 mA).

| Input | Base current | Forced gain (71 mA / Ib) | Result |
|---|---|---|---|
| 5 V (Arduino Uno, Nano) | (5 - 0.8) / 470 = 8.9 mA | 8 | hard on |
| 3.3 V (ESP32, STM32, Pico) | (3.3 - 0.8) / 470 = 5.3 mA, 5.2 mA after the pull-down | 14 | hard on |

8.9 mA is well inside what an AVR pin can give. 5.2 mA is fine for an ESP32 or STM32.
On a Raspberry Pi Pico the default pin drive is 4 mA. Either raise the drive strength to 8 mA in
code, or fit 1k instead of 470 ohm: (3.3 - 0.8) / 1k = 2.5 mA, a forced gain of about 28, which
still saturates a BC337 with good margin.

Saturated, the BC337 drops roughly 0.1 to 0.2 V at this current, so it dissipates about
0.2 V x 71 mA = 14 mW. No heatsink is needed.

## Pull-down: 10k

When a microcontroller resets, its pins float for a moment. A floating base can pick up enough
noise to half turn on the transistor and make the relay chatter. The 10k from base to ground holds
the base at 0 V until the pin drives it.

The input has to rise above about Vbe x (1 + 470 / 10k) = 0.6 V x 1.05 = **0.63 V** before any
collector current flows, so a low output (under 0.4 V) keeps the relay off with margin.

## Flyback diode: 1N4148

A coil resists any change in its current. When the transistor turns off, the 71 mA has nowhere to
go and the collector voltage jumps up, easily to tens or hundreds of volts. That can break down the
transistor and also put noise on the 5 V rail.

The diode sits across the coil, cathode to +5V. In normal use it is reverse biased and does nothing.
At turn off it gives the coil current a loop to die out in, so the collector is clamped at about
5 V + 0.8 V = **5.8 V**, far below the 45 V rating.

| Item | 1N4148 | Needed |
|---|---|---|
| Repetitive peak forward current | 400 mA or more (check your datasheet) | 71 mA, falling to zero in a few ms |
| Reverse voltage | 75 V or more | 5 V |

The cost of a plain diode is a slower release: the relay opens a few milliseconds later than it
would without the diode. That does not matter for switching lamps, fans or pumps. If you need a fast
release, add a zener in series with the diode; that is not on this board.

## Indicator LED

The LED and its 1k resistor sit in parallel with the coil, so the LED is lit exactly when the coil
has voltage across it.

LED current = (5 V - 1.9 V) / 1k = **3.1 mA**. That is plenty for a red 3 mm LED to be visible.

At turn off the collector briefly sits at 5.8 V, which puts about 0.8 V in reverse across the LED.
A typical LED allows 5 V reverse, so this is fine.

## Supply current

| Load | Current |
|---|---|
| One relay on | 71 mA coil + 3 mA LED = 74 mA |
| Both relays on | about 148 mA |
| Both off | close to zero (only capacitor leakage) |

Base current comes from the microcontroller pins, not from J3.
Use a 5 V supply rated at 300 mA or more. Powering the coils from a USB port or an Arduino 5V pin
works, but a separate 5 V supply keeps relay noise off the controller.
C1 (100 uF) and C2 (100 nF) sit across the 5 V input to soak up the current step when a relay pulls in.

## Contact side and the 24 V DC limit

The SRD relay body is marked with mains ratings, but this board is **not** designed for mains.
The gap between the contact tracks and the 5 V side is less than 1 mm in places and there are no
slots. Keep everything on the contacts at 24 V DC or less.

| Item | Value |
|---|---|
| Contact track width | 1.5 mm, 1 oz copper |
| Safe current for that track | about 3 A for a 10 degree rise (IPC-2221, outer layer) |
| Terminal blocks | 5.08 mm pitch, rated well above 3 A |
| Board limit | **24 V DC, 3 A** per contact |

DC is harder on relay contacts than AC because the arc does not stop by itself at a zero crossing.
For motors, solenoids or other coils on the contact side, put a diode across that load too.

## PCB layout

- 64 x 76 mm, 2 layers. Relays at the top, driver parts in the middle, power and control connectors at the bottom.
- Contact tracks (COM, NC, NO) are 1.5 mm wide on the top layer and stay above the driver area.
- Signal and coil tracks are 0.4 mm, the 5 V and GND tracks 0.5 mm.
- The bottom layer has a ground pour under the driver area only. It stops below the relays so no ground copper sits under the contact tracks.
- Every part is through-hole for easy hand soldering.
