# Bring-up and test plan

| # | Step | Expected result |
|---|---|---|
| 1 | Flash the firmware, open the terminal at 115200 | Lines start appearing |
| 2 | Leave all 4 inputs open | Noisy values, no crash. Floating inputs are unstable by nature. |
| 3 | Tie PA0 to GND | `ch=0 mean_mv` close to 0 |
| 4 | Tie PA0 to 3V3 | `ch=0 mean_mv` close to 3300 |
| 5 | Feed PA0 from a divider at about half of 3V3 | `mean_mv` near 1650, `max - min` small |
| 6 | Watch the `ovr` field for 1 minute | Stays 0 |
| 7 | Change TIM6 ARR to 199 | Block period doubles, output rate halves |
| 8 | Optional: toggle a spare pin in the half-complete callback and scope it | 25.6 ms between toggles |

If `ovr` increases, see the UART bandwidth warning in `architecture.md`.
