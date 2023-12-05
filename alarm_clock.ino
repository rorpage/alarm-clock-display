#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <TM1637Display.h>

TM1637Display display = TM1637Display(4, 0);
TM1637Display display2 = TM1637Display(14, 12);

const int BRIGHTNESS = 1;

const uint8_t C[] = {
  SEG_A | SEG_D | SEG_E | SEG_F   // C
};

const uint8_t F[] = {
  SEG_A | SEG_E | SEG_F | SEG_G   // F
};

const char* WIFI_SSID = "ssid";
const char* WIFI_PASS = "password";

const int HTTPS_PORT = 443;
const char* HOST = "alarm-clock-api.vercel.app";
const String DATA_URL = "/api/info?mini=true";

BearSSL::WiFiClientSecure client;

const int secondsBetweenChecks = 45000;
int timeSinceLastRead = 60001;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Running...");

  client.setInsecure();

  display.setBrightness(BRIGHTNESS);
  display2.setBrightness(BRIGHTNESS);

  connect();
}

void connect() {
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }

  while (WiFi.status() != WL_CONNECTED) {
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Failed to connect to WIFI. Please verify credentials: ");
      Serial.println();
      Serial.print("SSID: ");
      Serial.println(WIFI_SSID);
      Serial.print("Password: ");
      Serial.println(WIFI_PASS);
      Serial.println();
      Serial.println("Trying again...");
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      delay(10000);
    }

    delay(500);
    Serial.println("...");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void loop() {
  bool toReconnect = false;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Disconnected from WiFi");
    toReconnect = true;
  }

  if (toReconnect) {
    connect();
  }

  if (timeSinceLastRead > secondsBetweenChecks) {
    getWeatherAndTime();

    timeSinceLastRead = 0;
  }

  delay(1000);
  timeSinceLastRead += 1000;
}

void getWeatherAndTime() {
  if (client.connect(HOST, HTTPS_PORT)) {
    client.print(String("GET ") + DATA_URL + " HTTP/1.1\r\n" +
      "Host: " + HOST + "\r\n" +
      "User-Agent: rorpage_clock_ESP8266\r\n" +
      "Connection: close\r\n\r\n");

    delay(100);

    String response = client.readString();
    int body_position = response.indexOf("\r\n\r\n") + 4;
    String response_body = response.substring(body_position);

    StaticJsonDocument<256> jsonDocument;
    deserializeJson(jsonDocument, response_body);

    display.clear();
    display2.clear();

    int digits[4];
    digits[0] = jsonDocument["time_digits"][0];
    digits[1] = jsonDocument["time_digits"][1];
    digits[2] = jsonDocument["time_digits"][2];
    digits[3] = jsonDocument["time_digits"][3];

    // Time
    for (int i = 0; i < 4; i++) {
      display.showNumberDecEx(digits[i], 0b11100000, false, 1, i);
    }

    // Temperature
    int temperature = jsonDocument["temp_c"];
    display2.showNumberDec(temperature, false, 3, 0);
    display2.setSegments(C, 1, 3);
  } else {
    Serial.print("Error getting data");
  }
}
