# Test plan

Do these in order. Each step only needs a multimeter, a few jumper wires and the serial monitor.
Nothing here has been carried out yet for this design, so expect to find and fix the odd wiring mistake.

## 1. Before power

- [ ] Meter on continuity: +5V to GND is open.
- [ ] Every 1N4148 has its band towards J1.
- [ ] Every LED has its flat side (cathode) towards GND.

## 2. Power, no chips

- [ ] Plug the Nano into USB with the three sockets empty.
- [ ] Pin 16 of each socket reads 4.75 to 5.25 V. Pin 8 reads 0 V.
- [ ] /OE (pin 13) of U2 and U3 sockets is 0 V. /SRCLR (pin 10) is 5 V.

## 3. Firmware unit tests on a PC (optional but quick)

- [ ] In `firmware/test/`: `g++ -std=c++11 logic_check.cpp -I../cable_tester -o logic_check` then run `./logic_check`.
- [ ] It prints `all logic checks passed` and exits with 0.

## 4. Chips in, nothing on J1/J2

- [ ] Insert U2, U3, U4 with power off. Check the notch direction twice.
- [ ] Upload the sketch. All LEDs stay dark (no cable means nothing to show).
- [ ] USB current (if you have a USB meter) under 50 mA.
- [ ] Press the button: the first 2 wire LEDs light for one second. Each press adds one, after 8 it goes back to 2.
      Press until it shows 8 again.

## 5. Known cases with jumper wires

Use short Dupont jumpers between J1 and J2. Watch the LEDs and the serial monitor at 115200 baud.

| Jumpers | Expected LEDs | Expected serial line |
|---|---|---|
| 1-1 to 8-8 | All 8 on, green steady | `wires=8 PASS` and all `ok` |
| Remove 3-3 | LED 3 off, red | `3:open` |
| 1-2 and 2-1 (swap) | LEDs 1 and 2 blink, red | `1:crossed 2:crossed` |
| Add a jumper J2 pin 4 to J2 pin 5 | LEDs 4 and 5 blink, red | `4:short 5:short` |
| 1-8, 2-7, ... 8-1 | Wire LEDs off, green blinking | `REVERSED` |
| Button set to 4, jumpers 1-1 to 4-4 only | LEDs 1 to 4 on, green | `wires=4 PASS` |

## 6. Voltage check on the sense side

- [ ] With jumpers 1-1 to 8-8 and the serial monitor open, the far-end pins switch quickly so a meter shows an average.
      To see the static level, temporarily change the sketch to drive only wire 1 (call `writeRegisters(0, 0x01)` in `loop` and return).
- [ ] J2 pin 1 reads around 4.2 V. The other J2 pins read 0 V.
- [ ] Jumper all 8 J2 pins together. J2 pin 1 should still read above 3.5 V (the notes predict about 3.9 V).
- [ ] Put the original sketch back.

## 7. Real cable

- [ ] Fit RJ45 breakouts and test a known-good patch cable: green.
- [ ] If you have a crossover cable, it should show red with wires 1, 2, 3 and 6 blinking.
- [ ] Try the longest cable you have. If you see random blinking, raise `SETTLE_US` and re-test.

## Record

Write down the date, which steps passed and any changes you made. If a step fails, the wiring table in
`build-perfboard.md` and the netlist in `hardware/reports/netlist.xml` are the two places to compare against.
