# PS5 Telegram power-button controller

An ESP8266 polls a Telegram bot and uses an MG90S servo to press the PS5's
physical power button. The servo simulates a button press; it does not control
the console over its network API. The effect depends on the console's current
power state.

## Parts

| Part | Use |
|---|---|
| [NodeMCU V3 ESP8266 ESP-12E CH340](https://www.robotistan.com/nodemcu-lolin-esp8266-gelistirme-karti-usb-chip-ch340) | ESP8266 controller and USB serial interface |
| [NodeMCU ESP8266 Base Shield, CH340 compatible](https://www.robotistan.com/nodemcu-lolin-esp8266-base-shield) | Breakout/power base; the NodeMCU board is separate |
| [MG90S 180-degree servo](https://www.robotistan.com/mg90s-servo-motor) | Moves the button pusher |
| [Black mini breadboard](https://www.robotistan.com/siyah-mini-breadboard-breadboard-mini-self-adhesive-black) | Holds the assembly |
| [40-pin, 200 mm male-female jumper wires](https://www.robotistan.com/40-pin-ayrilabilen-disi-erkek-m-f-jumper-kablo-200-mm) | Servo connections |
| [1.5 m Micro-USB cable](https://www.robotistan.com/mikro-usb-kablo-1) | Programming and NodeMCU power |

The supplied build note also mentions an external 5 V phone adapter and an
eraser used as a soft contact pad. Neither appears in the purchase list; confirm
what was actually used in the assembled unit. The note calls the servo an SG90,
but the purchase list says MG90S; this project documents the purchased MG90S.

## Wiring

| Servo wire | Connect to |
|---|---|
| Brown (GND) | Ground on the base shield |
| Orange (signal) | D1 on the base shield (GPIO5 on NodeMCU) |
| Red (power) | A regulated 5 V supply, not the 3.3 V pin |
This mapping follows the wiring note you supplied; I have not visually verified
the physical connections.

Robotistan lists the MG90S at 4.8–6 V and 250–700 mA. Use a regulated supply
that can handle its startup/stall current. If the servo has a separate 5 V
supply, connect its ground to NodeMCU ground as well. Do not assume the shield's
VIN pin can supply the servo without checking how the shield is powered and
wired. Your build note reports resets when servo power shared the NodeMCU supply
and recommends separate 5 V; no current measurement was provided. Secure the
servo so it presses the PS5 button gently; the reported eraser pad softens
contact.

## Software and libraries

- Arduino IDE with the **ESP8266 by ESP8266 Community** board package.
  Board Manager URL: `https://arduino.esp8266.com/stable/package_esp8266com_index.json`
- Select **NodeMCU 1.0 (ESP-12E Module)** for the NodeMCU V3 ESP-12E board.
- **UniversalTelegramBot 1.3.0**. The local archive supplied for this build is
  `/Users/omerdikyol/Downloads/Universal-Arduino-Telegram-Bot-1.3.0.zip`.
  Install it with **Sketch → Include Library → Add .ZIP Library…**.
- **ArduinoJson 6.21.5** (Benoit Blanchon), installed through Library Manager.
  UniversalTelegramBot includes `ArduinoJson.h`, so this is required even
  though the sketch does not use ArduinoJson APIs directly.
- `ESP8266WiFi`, `WiFiClientSecure`, `Servo`, and `time` are supplied by the
  ESP8266 board platform; do not install a separate AVR Servo library for this
  board.

## Configure, compile, and upload

1. Open `ps5_controller.ino` in Arduino IDE.
2. In this directory, copy `secrets.example.h` to `secrets.h` and fill in the
   Wi-Fi SSID/password, Telegram bot token, and numeric Telegram chat ID. Use a
   2.4 GHz Wi-Fi network; ESP8266 does not support 5 GHz.
3. Keep `secrets.h` local. The repository ignores it. Never commit it or paste
   its contents into chat.
4. Select **NodeMCU 1.0 (ESP-12E Module)** and the board's serial port, then
   click **Upload**. Open Serial Monitor at **115200 baud** for startup output.
5. Open a private chat with the bot and send `/start` once. The bot cannot
   initiate a Telegram conversation before the user starts it.

The sanitized sketch compiled successfully for `esp8266:esp8266:nodemcuv2`
with ESP8266 platform 3.1.2, ArduinoJson 6.21.5, and UniversalTelegramBot
1.3.0. The compiler reported 32,416 / 80,192 bytes RAM (40%),
61,715 / 65,536 bytes IRAM (94%), and 384,368 / 1,048,576 bytes in flash
(36%). The owner confirmed receiving the Telegram status message and that the
bot works.

## Telegram commands and behavior

- `/ac`: moves the servo to 180 degrees for 600 ms, returns it to 10 degrees,
  then detaches it. This presses the physical PS5 button; it does not guarantee
  that the console powers on if it is already on.
- `/status` and `/durum`: reports local time, uptime, Wi-Fi RSSI, and IP address.
- `/start` and `/help`: returns the command list.

At boot the servo is moved to 10 degrees and detached. The ESP8266 then waits
until Wi-Fi connects, synchronizes time with NTP, and sends a boot report. If
time is not synchronized within five minutes, the sketch restarts. Wi-Fi
connection itself has no timeout. Messages from chat IDs other than the
configured `CHAT_ID` are silently ignored.

## Troubleshooting and security

- Compile error `ArduinoJson.h: No such file or directory`: install ArduinoJson
  6.21.5 from Library Manager.
- No reply to `/start`: check that the board is on 2.4 GHz Wi-Fi, that local
  `secrets.h` has the right values, and that the bot is being messaged from the
  configured chat ID. Check Serial Monitor at 115200 baud; repeated `.` means
  the sketch is still waiting for Wi-Fi, while repeated `*` means it is waiting
  for NTP time.
- `client.setInsecure()` disables TLS certificate verification. It is retained
  to match the currently working sketch, but permits a man-in-the-middle attack
  on the Telegram TLS connection. Add certificate validation only as a separate
  change and retest against the installed ESP8266 and Telegram libraries.
- If Wi-Fi credentials or the bot token are exposed, rotate them, update local
  `secrets.h`, and re-upload. Never commit the replacement values.
