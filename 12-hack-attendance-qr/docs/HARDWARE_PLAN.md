# Hardware plan (optional, design only, not built or tested in this package)

A laptop and projector are enough for the demo. This plan is for rooms with no projector: a small
USB-powered box on the teacher's desk that shows the rotating code.

## Block diagram

```
USB 5 V adapter or power bank -> ESP32 dev board --SPI--> 2.8 inch TFT (shows the QR)
                                     |
                                Wi-Fi: gets the session secret and the time from the server
```

## Suggested parts

| Part | Notes |
|---|---|
| ESP32 dev board with USB | 3.3 V logic, Wi-Fi for time sync |
| 2.8 inch SPI TFT, 240 x 320 (ILI9341 class) | A version 2 QR code at 6 px per module fits with a margin |
| USB 5 V adapter (certified) or a power bank | The only power source; nothing above 5 V in this build |
| Small plastic enclosure and a stand | Tilt the screen towards the class |

## Power budget (estimate, check against your board's datasheet)

ESP32 with Wi-Fi on averages around 100 to 150 mA, the TFT backlight around 50 to 80 mA. Call it
250 mA at 5 V, about 1.25 W. A 10000 mAh power bank (roughly 37 Wh at the cell, maybe 25 to 30 Wh
delivered) would run it for well over a day.

## Safety rules

1. USB 5 V only. Do not power the board from anything else, and do not open the power adapter.
2. Use a certified adapter or power bank. Cheap unbranded adapters are the main fire risk here.
3. Keep the TFT backlight pin on a resistor or transistor as the module's datasheet shows.
4. No exposed wiring on a desk where students sit; use the enclosure.

## Firmware outline

1. At boot, connect to Wi-Fi, get the time over NTP and the session secret from the server over HTTPS.
2. Every second, compute `window = floor(t / 15)`; when it changes, compute the token with the same
   HMAC as `common.py` and draw `RC1|session|window|token` as a QR code.
3. Show a small countdown bar so students know when the code is about to change.
4. If Wi-Fi drops, keep showing codes from the local clock; the server accepts them as long as the
   clock drift stays inside the 15 s grace (planned, not implemented).
