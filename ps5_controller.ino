#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Servo.h>
#include <ArduinoJson.h>
#include <time.h>
#include "secrets.h"

const int servoPin = D1;
const int pressAngle = 180;
const int restAngle = 10;

// POSIX TZ signs are reversed: GMT-3 means UTC+3.
#define MY_NTP_SERVER "pool.ntp.org"
#define MY_TZ "GMT-3"

Servo myServo;
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

unsigned long lastTimeBotRan;
const int botRequestDelay = 1000;
bool onlineAnnounced = false;

String getDateTime() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);

  if (timeinfo == nullptr || timeinfo->tm_year + 1900 < 2024) {
    return "[" + String(millis() / 1000) + "s]";
  }

  char buffer[30];
  strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", timeinfo);
  return String(buffer);
}

String getUptime() {
  unsigned long sec = millis() / 1000;
  int min = (sec / 60) % 60;
  int hr = (sec / 3600) % 24;
  int days = sec / 86400;
  return String(days) + " days " + String(hr) + " hr " +
         String(min) + " min";
}

void pressButton() {
  myServo.attach(servoPin);
  myServo.write(pressAngle);
  delay(600);
  myServo.write(restAngle);
  delay(500);
  myServo.detach();
}

String statusText() {
  String message = "System status\n";
  message += "--------------------------\n";
  message += "Time: " + getDateTime() + "\n";
  message += "Uptime: " + getUptime() + "\n";
  if (WiFi.status() == WL_CONNECTED) {
    message += "Wi-Fi signal: " + String(WiFi.RSSI()) + " dBm\n";
    message += "IP address: " + WiFi.localIP().toString() + "\n";
  } else {
    message += "Wi-Fi: not connected\n";
  }
  message += "Servo: idle\n";
  return message;
}

// USB serial commands, one per line: "press" or "status".
void handleSerial() {
  if (!Serial.available()) {
    return;
  }
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line == "press") {
    pressButton();
    Serial.println("OK press");
  } else if (line == "status") {
    Serial.print(statusText());
    Serial.println("OK status");
  } else if (line.length() > 0) {
    Serial.println("ERR unknown command: " + line);
  }
}

void setup() {
  Serial.begin(115200);

  // Return the servo to its resting position after boot.
  delay(2000);
  myServo.attach(servoPin);
  myServo.write(restAngle);
  delay(500);
  myServo.detach();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  client.setInsecure();

  configTime(MY_TZ, MY_NTP_SERVER);
  Serial.setTimeout(100);
  Serial.println("\nReady. Serial commands: press, status");
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chatId = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    // Ignore messages from chats other than the configured owner chat.
    if (chatId != CHAT_ID) {
      continue;
    }

    if (text == "/press") {
      bot.sendMessage(chatId, "Pressing the PS5 power button...", "");

      pressButton();
      bot.sendMessage(chatId, "Button press complete.", "");
    } else if (text == "/status") {
      bot.sendMessage(chatId, statusText(), "");
    } else if (text == "/start" || text == "/help") {
      String message = "Available commands:\n\n";
      message += "/press - Press the PS5 power button\n";
      message += "/status - Show device status";
      bot.sendMessage(chatId, message, "");
    }
  }
}

void loop() {
  handleSerial();

  // Telegram is optional: skip it while Wi-Fi is down. The ESP8266 reconnects
  // on its own after WiFi.begin().
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (!onlineAnnounced) {
    onlineAnnounced = bot.sendMessage(CHAT_ID, "System is online. IP: " +
                                      WiFi.localIP().toString(), "");
  }

  if (millis() - lastTimeBotRan > botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}
