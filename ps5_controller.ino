#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Servo.h>
#include <ArduinoJson.h>
#include <time.h>
#include "secrets.h"

// --- SETTINGS ---
const int servoPin = D1;
const int acikAci = 180;
const int kapaliAci = 10;

// Turkey time (UTC+3; POSIX TZ signs are reversed).
#define MY_NTP_SERVER "pool.ntp.org"
#define MY_TZ "GMT-3"

Servo myServo;
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

unsigned long lastTimeBotRan;
const int botRequestDelay = 1000;
String bootLog = "";

String getTarihSaat() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);

  if (timeinfo == nullptr || timeinfo->tm_year + 1900 < 2024) {
    return "[" + String(millis() / 1000) + "sn]";
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
  return String(days) + " gün " + String(hr) + " sa " + String(min) + " dk";
}

void lojEkle(String mesaj) {
  bootLog += getTarihSaat() + " -> " + mesaj + "\n";
}

void setup() {
  Serial.begin(115200);

  // Return the servo to its resting position after boot.
  delay(2000);
  myServo.attach(servoPin);
  myServo.write(kapaliAci);
  delay(500);
  myServo.detach();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  client.setInsecure();

  lojEkle("Sistem Başlatıldı. Wi-Fi aranıyor...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nModeme Baglandi.");

  configTime(MY_TZ, MY_NTP_SERVER);
  lojEkle("Modeme bağlandı, İnternet bekleniyor...");

  int beklemeSayaci = 0;
  while (time(nullptr) < 1700000000) {
    delay(1000);
    Serial.print("*");
    beklemeSayaci++;

    if (beklemeSayaci > 300) {
      ESP.restart();
    }
  }

  lojEkle("İNTERNET GELDİ! IP: " + WiFi.localIP().toString());

  // Telegram requires the user to start the bot chat before it can message them.
  bot.sendMessage(CHAT_ID, "📅 SİSTEM GERİ GELDİ!\n\n" + bootLog, "");
  bootLog = "";
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    // Ignore messages from all chats except the configured owner chat.
    if (chat_id != CHAT_ID) {
      continue;
    }

    if (text == "/ac") {
      bot.sendMessage(chat_id, "🎮 PS5 Açılıyor...\nSaat: " + getTarihSaat(), "");

      myServo.attach(servoPin);
      myServo.write(acikAci);
      delay(600);
      myServo.write(kapaliAci);
      delay(500);
      myServo.detach();

      bot.sendMessage(chat_id, "✅ Tuşa basıldı.", "");
    } else if (text == "/durum" || text == "/status") {
      String mesaj = "📊 *SİSTEM DURUM RAPORU*\n";
      mesaj += "--------------------------\n";
      mesaj += "🕒 Tarih: " + getTarihSaat() + "\n";
      mesaj += "⏳ Çalışma Süresi: " + getUptime() + "\n";
      mesaj += "📡 Wi-Fi Gücü: " + String(WiFi.RSSI()) + " dBm\n";
      mesaj += "🌐 IP Adresi: " + WiFi.localIP().toString() + "\n";
      mesaj += "🤖 Servo Durumu: Beklemede (Pasif)\n";

      bot.sendMessage(chat_id, mesaj, "");
    } else if (text == "/start" || text == "/help") {
      String mesaj = "Merhaba! Komut Listesi:\n\n";
      mesaj += "👉 /ac - PS5'i açar\n";
      mesaj += "👉 /status - Sistem durumunu gösterir";
      bot.sendMessage(chat_id, mesaj, "");
    }
  }
}

void loop() {
  // Reconnect to Wi-Fi after a drop.
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Kopma algılandı!");
    WiFi.disconnect();
    WiFi.reconnect();

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
    delay(2000);
  }

  if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}
