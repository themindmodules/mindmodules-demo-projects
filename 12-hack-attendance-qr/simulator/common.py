"""Settings and the rotating QR token, shared by sim.py and analyze.py.

The classroom screen shows a QR code that changes every WINDOW_S seconds.
The code carries:  RC1|<session_id>|<window>|<token>
where token = first 8 hex characters of HMAC-SHA256(secret, "<session_id>:<window>").

Only the server knows the secret, so a phone cannot make a valid token for a
window it has not seen. A screenshot still works, but only for a short time,
which is what the STALE-TOKEN rule checks.
"""
import hashlib
import hmac

SESSION_ID = "EE201-L07"    # course code and lecture number for the demo
WINDOW_S = 15               # QR code changes every 15 s
GRACE_S = 15                # accept a token up to one extra window late (network and camera delay)
CLOSE_S = 600               # on-time scanning closes 10 minutes after the start
LATE_END_S = 720            # the code stays on screen 2 more minutes for late arrivals

# Demo value only. A real server keeps a fresh random secret per session and never ships it.
DEMO_SECRET = b"rollcall-demo-only-change-me"


def make_token(secret, session_id, window):
    msg = ("%s:%d" % (session_id, window)).encode("ascii")
    return hmac.new(secret, msg, hashlib.sha256).hexdigest()[:8]


def qr_payload(session_id, window, secret=DEMO_SECRET):
    return "RC1|%s|%d|%s" % (session_id, window, make_token(secret, session_id, window))


def mmss(seconds):
    s = int(seconds)
    return "%02d:%02d" % (s // 60, s % 60)
