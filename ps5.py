#!/usr/bin/env python3
"""Control the PS5 from a Linux host on the same LAN.

    ps5.py status    PS5 power state over the network
    ps5.py wake      wake from Rest Mode over the network
    ps5.py standby   put into Rest Mode over the network
    ps5.py press     press the power button with the servo (USB serial)
    ps5.py board     servo board status (USB serial)

Network commands need a one-time Remote Play pairing (see README).
"""
import asyncio
import sys
import warnings

PS5_HOST = "192.168.1.163"  # give the PS5 a DHCP reservation so this stays valid
SERIAL_PORT = "/dev/ttyUSB0"


def serial_cmd(cmd):
    import serial

    # Default open asserts DTR and RTS together, which leaves the NodeMCU running.
    # Forcing them low before open resets the board (tested on the FT232R variant).
    try:
        s = serial.Serial(SERIAL_PORT, 115200)
    except serial.SerialException as e:
        sys.exit(f"servo board not reachable: {e}")
    # Terminate any junk the board already buffered (e.g. after a replug),
    # then drop its "ERR unknown command" reply before sending ours.
    s.write(b"\n")
    s.timeout = 0.3
    s.read(4096)
    s.timeout = 5
    s.write(cmd.encode() + b"\n")
    while True:
        line = s.readline().decode(errors="replace").rstrip()
        if not line:
            sys.exit("timeout: no reply from board")
        print(line)
        if line.startswith(("OK", "ERR")):
            return line.startswith("OK")


def device():
    warnings.filterwarnings("ignore")  # pyremoteplay warns that av (video) is missing
    from pyremoteplay import RPDevice
    from pyremoteplay.profile import Profiles

    d = RPDevice(PS5_HOST)
    # it skips a query now and then, most of all just after changing state
    if not any(d.get_status() for _ in range(5)):
        sys.exit(f"PS5 not reachable at {PS5_HOST}")
    return d, Profiles.load().usernames[0]


def main():
    cmd = sys.argv[1] if len(sys.argv) > 1 else "status"
    if cmd in ("press", "board"):
        return serial_cmd("status" if cmd == "board" else "press")
    if cmd not in ("status", "wake", "standby"):
        sys.exit(__doc__)

    d, user = device()
    if cmd == "status":
        print("on" if d.is_on else "rest mode")
        return True
    if cmd == "wake":
        if d.is_on:
            print("already on")
            return True
        d.wakeup(user)
        ok = d.wait_for_wakeup(60)
        print("on" if ok else "wake sent, still not on after 60 s")
        return ok
    if not d.is_on:
        print("already in rest mode")
        return True
    async def standby():
        try:
            return await d.standby(user)
        finally:
            d.disconnect()  # inside the loop: after asyncio.run it fails with "Event loop is closed"

    # pyremoteplay 0.7.6 reports failure on firmware 14.23 ("Version not
    # accepted" during the session handshake), yet the console goes to rest
    # mode within 2 s (measured 2026-11-19). Judge by the console's own state.
    asyncio.run(standby())
    for _ in range(20):
        if d.get_status() and not d.is_on:
            print("rest mode")
            return True
    print("standby failed: still on")
    return False


if __name__ == "__main__":
    sys.exit(0 if main() else 1)
