# Safety and failure modes

| Risk | What protects you | What you still must do |
|---|---|---|
| Sensor fails or reads wrong, pump runs forever | `maxPumpMs` stops every run at 10 s; `lockoutMs` spaces runs | Keep the water outlet over a tray, not electronics |
| Code crashes with the pump on | MOSFET gate has a 100 kohm pulldown, so it turns off if the pin floats | Fit the pulldown, do not skip it |
| Pump back-EMF damages the MOSFET or Arduino | Flyback diode across the pump | Check diode polarity before powering |
| Water on the electronics | Nothing in software | Mount the board above the water line, keep the sensor cable drip-looped |
| Pump supply shorts to Arduino 5 V | Separate supply, only grounds joined | Never connect the two positive rails |
| Sensor corrodes over months | Capacitive type (not resistive) | Seal the top edge of the sensor with nail varnish or heat-shrink |

This project switches low-voltage DC only. Do not adapt it to mains-powered pumps without a
proper relay module, an enclosure and someone qualified to review it.
