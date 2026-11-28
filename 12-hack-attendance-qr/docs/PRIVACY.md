# Privacy rules

Attendance data is personal data about students, many of them minors in schools. Treat it that way
even in a hackathon demo. Judges often ask about this, and a clear answer scores well.

## What we store

| Data | Why | Kept for |
|---|---|---|
| Student ID (roll number) | To mark attendance | As long as the institute keeps attendance |
| A random device ID made by the scanning page | Device binding | Until the student re-registers |
| Scan time, window number, token | To check the rules and settle disputes | One term, then delete the raw rows |
| Final status per lecture | The attendance record itself | Per institute policy |

## What we never collect

- Location (GPS, Wi-Fi lists, cell towers)
- Photos or face data
- Phone number, contacts, IMEI or other hardware identifiers
- Anything about the phone beyond the random device ID the page stores itself

## Rules for the demo and for a pilot

1. The demo uses only synthetic IDs (`S001` to `S060`). Never put a real class list in the repo.
2. A flag is a reason to look, not a verdict. The teacher talks to the student before any action.
3. Students can see their own scan history and the reason for any flag.
4. Keep the session secret on the server only. Rotate it every lecture.
5. If you run a pilot with real students, get permission from the institute first and tell students
   what is collected, in plain words, before the first scan.
