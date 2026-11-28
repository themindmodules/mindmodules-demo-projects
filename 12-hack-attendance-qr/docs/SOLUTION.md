# Solution design

```
 [projector or ESP32 display]          [student phone]                 [server]
  shows RC1|session|window|token  -->  scans with a web page  --HTTPS-->  checks token, age, device
  new code every 15 s                  sends id + device id               analyze.py rules
                                                                              |
                                                                  teacher dashboard: list + flags
```

## Parts and why

| Part | Why it exists |
|---|---|
| Rotating code | A photo of the code stops working after about 30 s |
| HMAC token | Phones cannot make a valid code for a window they did not see; only the server holds the secret |
| Device binding | Each student registers one phone at the start of term; a second student on that phone stands out |
| Rule engine | Every decision has a one-line reason the teacher can read out |
| Dashboard | The class list at a glance, plus a short list of things to check |

## Token maths

The window number is `floor(t / 15)`. The token is the first 8 hex characters of
`HMAC-SHA256(secret, "session:window")`, which is 32 bits. Guessing one blindly is a 1 in 4.3 billion
chance per try, and the server can rate-limit tries per device. 8 characters keeps the QR small
enough to scan from the back row.

A scan is accepted if it arrives no more than 15 + 15 = 30 s after the window started. Honest scans
take 0 to about 19 s (up to 15 s waiting inside the window, plus a few seconds of camera and network
delay), so the 15 s grace leaves room for a slow phone without letting a forwarded screenshot through.

## The rules (in the order they run)

| Rule | Trigger | Effect |
|---|---|---|
| INVALID-TOKEN | Wrong session, or token does not match the window | Row rejected |
| STALE-TOKEN | Token older than 30 s | Row rejected |
| FOREIGN-DEVICE | Phone registered to another student | Row rejected |
| NEW-DEVICE | Phone not registered to anyone | Accepted, status REVIEW |
| DUPLICATE | Student already has an accepted scan | Ignored, no penalty |
| LATE | First good scan after scanning closes | Status LATE |
| SHARED-DEVICE | One phone used for more than one student | All but the owner become SUSPECT |

A student with at least one clean scan is marked. A student with only rejected scans, or one who was
scanned from someone else's phone, is SUSPECT and is not marked until the teacher decides.

## Why not machine learning or GPS?

One lecture of data is nothing to train on, and every flag must be explainable to a student who
disputes it. GPS indoors is unreliable, easy to fake on a rooted phone, and collects far more personal
data than attendance needs. Rules plus a short code lifetime cover the common tricks.

## Limitations (say these out loud in the pitch)

- A friend in the room on a live video call can relay the code inside 30 s. Device binding stops them
  using the friend's phone, but not the absent student's own phone at home. The fix for a high-stakes
  class is a second factor in the room, like a short Bluetooth beacon check or a random roll call of
  two names.
- A student can register a friend's spare phone at the start of term. Registration should happen in
  person, once.
- Thresholds were chosen for a 60-student room on synthetic data. A 300-seat hall needs a longer
  window and a check of real scan times.
