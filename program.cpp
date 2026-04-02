#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <DHT.h>
#include <stddef.h>

#define DHTPIN D1
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// --- Wi-Fi (replace with your network) ---
char ssid[] = "NAME OF WIFI";
char password[] = "WIFI PASSWORD";

// --- Telegram ---
// Chat IDs: https://t.me/chatidx_bot — add every user/group that may use the bot.
static const char *kTelegramChatIds[] = {
    "YOUR CHAT ID",
    // "SECOND CHAT ID",
};
static const size_t kTelegramChatIdCount =
    sizeof(kTelegramChatIds) / sizeof(kTelegramChatIds[0]);

static const char *kBotToken = "BOTTOKEN";

WiFiClientSecure client;
UniversalTelegramBot bot(kBotToken, client);

static const unsigned long kBotPollMs = 1000;
static const unsigned long kWiFiConnectTimeoutMs = 60000;

static unsigned long lastBotPoll = 0;

static bool isAuthorizedChat(const String &chatId) {
  for (size_t i = 0; i < kTelegramChatIdCount; i++) {
    if (chatId == kTelegramChatIds[i]) {
      return true;
    }
  }
  return false;
}

static bool commandIs(const String &text, const char *cmd) {
  if (!text.length()) {
    return false;
  }
  if (!text.startsWith(cmd)) {
    return false;
  }
  const size_t n = strlen(cmd);
  if (text.length() == n) {
    return true;
  }
  // Telegram desktop/mobile often sends "/start@YourBot"
  return text.charAt(n) == '@';
}

static void sendSensorReadings(const String &chatId) {
  const float h = dht.readHumidity();
  const float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    bot.sendMessage(chatId, "Sensor read failed. Check wiring and try again.", "");
    return;
  }
  String msg = "Temperature: ";
  msg += String(t, 1);
  msg += " °C\nHumidity: ";
  msg += String(h, 1);
  msg += " %\n\nBuilt by @mcnaveen";
  bot.sendMessage(chatId, msg, "");
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    const String chatId = String(bot.messages[i].chat_id);
    const String text = bot.messages[i].text;
    const String fromName = bot.messages[i].from_name;

    const bool authorized = isAuthorizedChat(chatId);

    if (!authorized) {
      bot.sendMessage(chatId, "Unauthorized user.", "");
      continue;
    }

    if (commandIs(text, "/start")) {
      String welcome = "Welcome, " + fromName + "!\n\n";
      welcome += "Room monitor by @mcnaveen\n\n";
      welcome += "/temperature — temperature (°C)\n";
      welcome += "/humidity — humidity (%)\n";
      welcome += "/status — temperature and humidity\n";
      bot.sendMessage(chatId, welcome, "");
      continue;
    }

    if (commandIs(text, "/temperature")) {
      const float t = dht.readTemperature();
      if (isnan(t)) {
        bot.sendMessage(chatId, "Temperature read failed.", "");
        continue;
      }
      String msg = "Temperature: ";
      msg += String(t, 1);
      msg += " °C\n\nBuilt by @mcnaveen";
      bot.sendMessage(chatId, msg, "");
      continue;
    }

    if (commandIs(text, "/humidity")) {
      const float h = dht.readHumidity();
      if (isnan(h)) {
        bot.sendMessage(chatId, "Humidity read failed.", "");
        continue;
      }
      String msg = "Humidity: ";
      msg += String(h, 1);
      msg += " %\n\nBuilt by @mcnaveen";
      bot.sendMessage(chatId, msg, "");
      continue;
    }

    if (commandIs(text, "/status")) {
      sendSensorReadings(chatId);
      continue;
    }

    bot.sendMessage(
        chatId,
        "Unknown command. Send /start for a list.",
        "");
  }
}

static bool connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > kWiFiConnectTimeoutMs) {
      Serial.println("\nWi-Fi connection timed out.");
      return false;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  return true;
}

void setup() {
  Serial.begin(115200);
  // After upload/reset the host UART may miss the first bytes; brief wait + banner helps.
  delay(200);
  Serial.println();
  Serial.println("--- ESP8266 DHT11 Telegram: setup ---");

  dht.begin();

  client.setInsecure();
  client.setBufferSizes(2048, 512);

  if (!connectWifi()) {
    Serial.println("Restarting in 10 s to retry Wi-Fi...");
    delay(10000);
    ESP.restart();
  }

  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  delay(1000);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi lost, reconnecting...");
    if (!connectWifi()) {
      delay(5000);
      return;
    }
    delay(1000);
  }

  if (static_cast<long>(millis() - lastBotPoll) < static_cast<long>(kBotPollMs)) {
    return;
  }
  lastBotPoll = millis();

  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  while (numNewMessages) {
    Serial.println("Telegram: handling messages");
    handleNewMessages(numNewMessages);
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  }
}
