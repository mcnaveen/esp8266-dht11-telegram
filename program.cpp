#include <ESP8266WiFi.h>

#include <WiFiClientSecure.h>

#include <UniversalTelegramBot.h>

#include <DHT.h>

#define DHTPIN D1

#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// Initialize WiFi connection

char ssid[] = "NAME OF WIFI"; // your network SSID (name)
char password[] = "WIFI PASSWORD"; // your network password

// Initialize Telegram BOT

#define BOTtoken "BOTTOKEN" // your telegram bot token

WiFiClientSecure client;

UniversalTelegramBot bot(BOTtoken, client);

//Checks for new messages every 1 second.

int botRequestDelay = 1000;

unsigned long lastTimeBotRan;

void handleNewMessages(int numNewMessages) {

  Serial.println("handleNewMessages");

  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++) {

    String chat_id = String(bot.messages[i].chat_id);

    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;

    // Your Chat ID get it from here: https://t.me/chatidx_bot

    if (chat_id == "YOUR CHAT ID") {

      if (text == "/temperature") {

        int t = dht.readTemperature();

        String temp = "Temperature : ";

        temp += int(t);

        temp += "°C\n\nBuilt by @mcnaveen";

        Serial.println(from_name);

        bot.sendMessage(chat_id, temp, "");

      }

      if (text == "/humidity") {

        int h = dht.readHumidity();

        String temp = "Humidity: ";

        temp += int(h);

        temp += "%\nBuilt by @mcnaveen";

        bot.sendMessage(chat_id, temp, "");

      }

    } else {
      bot.sendMessage(chat_id, "Unauthorized User", "");
    }

    if (text == "/start") {

      String welcome = "Welcome  " + from_name + "\n\nThis bot is Built by @mcnaveen to find room temperature\n\n Choose your option\n";

      welcome += "/temperature : Get Temperature\n";

      welcome += "/humidity : Get Humiditiy\n";

      bot.sendMessage(chat_id, welcome, "Markdown");

    }

  }

}

void setup() {

  Serial.begin(115200);

  dht.begin();

  client.setInsecure();

  // WiFo Connected

  WiFi.mode(WIFI_STA);

  WiFi.disconnect();

  delay(100);

  // Attempt to connect to WiFi network:

  Serial.print("Connecting Wifi: ");

  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {

    Serial.print(".");

    delay(500);

  }

  Serial.println("");

  Serial.println("WiFi connected");

  Serial.print("IP address: ");

  Serial.println(WiFi.localIP());

}

void loop() {

  int t = dht.readTemperature();

  int h = dht.readHumidity();

  if (millis() > lastTimeBotRan + botRequestDelay) {

    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {

      Serial.println("Got Response");

      handleNewMessages(numNewMessages);

      numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    }

    lastTimeBotRan = millis();

  }

}