# Hardware plan (design only, not built or tested in this package)

## Block diagram

```
Mains in -> [isolated energy-metering module] --UART/I2C--> ESP32 --Wi-Fi--> server
                         |
                  load out (appliance)
```

## Suggested parts

| Part | Notes |
|---|---|
| ESP32 dev board | Wi-Fi, 3.3 V logic |
| Isolated single-phase energy metering module (for example a PZEM-004T v3 class module) | Gives voltage, current, power over UART; the mains side is already isolated from the logic side |
| 5 V supply module for the ESP32 | Use a certified, enclosed adapter |
| Enclosure, strain relief, fuse | Mandatory, not optional |

## Safety rules (read before touching mains)

1. If you are not experienced with mains wiring, **demo with the simulator**, or measure a low-voltage DC
   load instead. A working demo is not worth an injury.
2. Never work on the circuit while it is plugged in.
3. Use an enclosure with no exposed conductors, a fuse on the live side, and proper strain relief.
4. Have a qualified electrician review the final build before powering it from the wall.
5. Check the module's own datasheet for its isolation rating and wiring instructions; follow them, not this file.

## Firmware outline

1. Read voltage, current, power from the module once per second; average to one sample per minute.
2. Append `minute,time,voltage_v,current_a,power_w` (same columns as `telemetry.csv`).
3. Send each row over MQTT or HTTP to the server; buffer to flash if Wi-Fi is down (planned).
