# 3-minute pitch (6 slides, 30 seconds each)

**Slide 1: Hook (problem).**
"A shop owner's fridge compressor stopped cycling at 2 PM. They found out at 9 PM when the stock was
spoiled. The electricity meter had recorded everything and told them nothing."

**Slide 2: Why existing tools miss it.**
"Smart meters give totals. Plug monitors give a number. Nobody tells you when a pattern is abnormal."

**Slide 3: Our solution.**
"GridWatch: a low-cost meter that watches voltage and load and sends plain-language alerts like
'Fridge has been on for 90 minutes without cycling'."

**Slide 4: Live demo (the most important slide).**
Open the dashboard. Point at the power chart, then at the detected events table. Say:
"This is a simulated day. At 14:20 we injected a failing compressor. GridWatch flagged it at 15:40,
and flagged the voltage sag at 18:05." (Be clear that the data is simulated. If you have a real
measurement running, show that instead and say so.)

**Slide 5: How it works and why it is simple.**
Hardware block diagram, then the three detectors in one line each. "We chose rules we can explain over a
model we cannot train on one day of data."

**Slide 6: Roadmap and ask.**
"Next: learn each appliance's normal cycle per site, add SMS alerts, pilot with three shops.
We need a pilot partner and sensor-safety review."

## Delivery tips

- Rehearse the demo three times; run it offline from `file://` so Wi-Fi cannot break it.
- Keep the laptop on charger and the dashboard pre-opened on the final slide's tab.
- Do not state market-size numbers you cannot source.
