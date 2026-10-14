# PS5 power button, controlled by Telegram

A NodeMCU ESP8266 polls a Telegram bot and moves an MG90S servo to press the
PS5's physical power button. Send `/press` to press it, or `/status` to check
the controller.

> The servo presses the console's physical button. It cannot read the PS5's
> power state, and pressing the button may have a different effect depending on
> whether the console is on or off.

![Wiring diagram for the NodeMCU, MG90S servo, and external 5 V supply](wiring.svg)

## Commands

| Telegram command | Action |
|---|---|
| `/press` | Move the servo to 180 degrees for 600 ms, then return it to 10 degrees |
| `/status` | Report local time, uptime, Wi-Fi signal, and IP address |
| `/start`, `/help` | Show the command list |

The bot accepts commands only from the `CHAT_ID` configured in `secrets.h`.
Open a private chat and send `/start` once so Telegram allows the bot to reply.

## Parts

| Part | Use |
|---|---|
| [NodeMCU V3 ESP8266 ESP-12E CH340](https://www.robotistan.com/nodemcu-lolin-esp8266-gelistirme-karti-usb-chip-ch340) | Controller and USB serial interface |
| [NodeMCU ESP8266 Base Shield](https://www.robotistan.com/nodemcu-lolin-esp8266-base-shield) | Breakout for the NodeMCU CH340 board. The board is sold separately. |
| [MG90S 180-degree servo](https://www.robotistan.com/mg90s-servo-motor) | Presses the PS5 button |
| [Black mini breadboard](https://www.robotistan.com/siyah-mini-breadboard-breadboard-mini-self-adhesive-black) | Holds the assembly |
| [40-pin, 200 mm male-female jumper wires](https://www.robotistan.com/40-pin-ayrilabilen-disi-erkek-m-f-jumper-kablo-200-mm) | Connect the servo |
| [1.5 m Micro-USB cable](https://www.robotistan.com/mikro-usb-kablo-1) | Program and power the NodeMCU |

The written build notes also mention a 5 V phone adapter and an eraser used as
a soft contact pad. Neither is in the purchase list. The notes once called the
servo an SG90; the purchased part is the MG90S.

## Wiring

The diagram follows the wiring details in the build notes. I have not inspected
the physical connections. Servo wire colors shown are brown for ground, orange
for signal, and red for power.

Robotistan lists the MG90S for 4.8-6 V at 250-700 mA. Use a regulated 5 V supply
that can handle the servo's startup current. Do not power it from the NodeMCU's
3.3 V pin. If the servo has a separate supply, join its ground to NodeMCU GND.
The build notes report resets when servo power shared the NodeMCU supply and
recommend a separate 5 V source. They do not give a current measurement. Check
the Base Shield's VIN wiring before using it as the servo supply.

## Build and upload

1. Install the **ESP8266 by ESP8266 Community** board package in Arduino IDE.
   Board Manager URL:
   `https://arduino.esp8266.com/stable/package_esp8266com_index.json`
2. Install [Universal-Arduino-Telegram-Bot](https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot)
   version **1.3.0**. Download the [V1.3.0 ZIP](https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot/archive/refs/tags/V1.3.0.zip)
   and use **Sketch > Include Library > Add .ZIP Library**.
3. Install **ArduinoJson 6.21.5** by Benoit Blanchon from Library Manager.
   UniversalTelegramBot includes `ArduinoJson.h`, so this is a required
   dependency.
4. Copy `secrets.example.h` to `secrets.h`. Add the 2.4 GHz Wi-Fi SSID and
   password, the Telegram bot token, and the numeric chat ID. Keep `secrets.h`
   local. Git ignores it.
5. Open `ps5_controller.ino`, select **NodeMCU 1.0 (ESP-12E Module)** and the
   serial port, then click **Upload**. Open Serial Monitor at **115200 baud**.

`ESP8266WiFi`, `WiFiClientSecure`, `Servo`, and `time` come with the ESP8266
board package. Do not install an AVR-specific Servo library.

The sketch compiled with ESP8266 platform 3.1.2, ArduinoJson 6.21.5, and
UniversalTelegramBot 1.3.0 for `esp8266:esp8266:nodemcuv2`.
The latest compile used 30,960 / 80,192 bytes RAM (38%), 61,715 / 65,536
bytes IRAM (94%), and 379,144 / 1,048,576 bytes in flash (36%).

## How it starts

At boot, the servo moves to its resting position at 10 degrees and detaches.
The controller joins Wi-Fi, waits for NTP time, then sends a boot message.
It restarts if time has not synchronized after five minutes. Wi-Fi connection
has no timeout. After startup, the loop polls Telegram once per second.

## Troubleshooting and security

- `ArduinoJson.h: No such file or directory`: install ArduinoJson 6.21.5.
- Repeated `.` in Serial Monitor means the board is waiting for Wi-Fi.
  Repeated `*` means it is waiting for NTP time. ESP8266 supports 2.4 GHz Wi-Fi,
  not 5 GHz.
- `client.setInsecure()` disables TLS certificate verification. The sketch
  keeps this setting to match the tested firmware, but it leaves Telegram
  connections vulnerable to a man-in-the-middle attack.
- If Wi-Fi credentials or the bot token are exposed, rotate them, update
  `secrets.h`, and upload again. Never commit `secrets.h`.
