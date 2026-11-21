# Judge Q&A (prepare honest answers)

**Is this real data?**
No, the demo uses a simulator with two injected faults. The analyzer reads a plain CSV, so real
readings drop in without code changes. Say this before they ask.

**Why not machine learning?**
One site, one day: nothing to train on. Rules are explainable and testable. ML comes after weeks of data per site.

**How accurate is the measurement?**
Not billing grade. Accuracy depends on the sensor module chosen and calibration. The goal is detecting
abnormal patterns, not metering.

**Isn't this just a smart plug?**
A smart plug shows consumption. GridWatch interprets patterns and alerts on abnormal behaviour.

**Is it safe to build?**
Mains work is dangerous. The design uses an isolated, pre-certified sensing module, an enclosure, and no
exposed mains. Anyone building it should have a qualified person review the wiring.

**What happens when Wi-Fi drops?**
The ESP32 buffers readings and sends them when the link returns (planned). Not implemented in this demo.

**Business model?**
Do not invent numbers. Options to mention: device sale, small monthly alert subscription for shops,
or installer partnerships. Say these are hypotheses to validate in a pilot.

**What did you build in 24 hours versus plan?**
Answer exactly: demo software built and tested on simulated data; hardware is designed, not built (or
say what you did build).
