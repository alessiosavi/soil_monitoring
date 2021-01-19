#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <time.h>

#define DHTPIN1 12    // D6
#define DHTPIN2 14    // D5
#define ANALOG_PIN A0 // D1

const char *ssid = "TIM-29854979";
const char *password = "billgatesfinocchio";

DHT dht[] = {
    {DHTPIN1, DHT11},
    {DHTPIN2, DHT11},
};
WiFiServer server(80);

int analogRead() { return analogRead(ANALOG_PIN); }

void led_off() { digitalWrite(LED_BUILTIN, HIGH); }
void led_on() { digitalWrite(LED_BUILTIN, LOW); }

// flash_n_times is delegated to produce N flash from the standard LED of the
// eps8266
void flash_n_times(int n) {
  led_off();
  for (int i = 0; i < n; i++) {
    led_on();
    delay(50);
    led_off();
    delay(200);
  }
}

void connect() {
  Serial.print("Connecting to " + String(ssid) + " ...");
  WiFi.begin(ssid, password); // Connect to the network
  // Turn the LED on by making the voltage LOW
  digitalWrite(LED_BUILTIN, LOW);
  int i = 0;
  // Wait for the Wi-Fi to connect
  while (WiFi.status() != WL_CONNECTED) {
    delay(5000);
    Serial.print(++i);
    Serial.print(' ');
  }
  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  // Send the IP address of the ESP8266 to the computer
  Serial.println(WiFi.localIP());
  // Turn the LED off by making the voltage HIGH
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.print(ESP.getFreeHeap());
}

void setup() {
  Serial.begin(9600);
  // Initialize the LED_BUILTIN pin as an output
  pinMode(LED_BUILTIN, OUTPUT);
  connect();
  Serial.println("DHT BEGIN");
  for (auto &sensor : dht) {
    sensor.begin();
    delay(100);
  }
  Serial.println("DHT INITIALIZED");
  pinMode(ANALOG_PIN, INPUT);
}

// Declare an object of class HTTPClient
HTTPClient http;
float temp = 0;
float hum = 0;
int light = -1;
// int soil = -1;
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WIFI DISCONNECTED! Trying to reconnect");
    connect();
  }

  for (auto &sensor : dht) {
    // Read and send temperature
    temp = sensor.readTemperature(false, true);
    if (isnan(temp)) {
      flash_n_times(3);
    } else {
      send_data(temp, "temp");
    }

    // Read and send the humidity
    hum = sensor.readHumidity(true);
    if (isnan(hum)) {
      flash_n_times(3);
    } else {
      send_data(hum, "hum");
    }
  }

  // Read and send light
  light = analogRead();
  Serial.println(light, DEC);
  send_data(light, "light");
  //
  //    // Read and send soil
  //    soil = analogSoil();
  //    Serial.println(soil);
  //    send_data(soil , "soil");

  delay(1000);
}

void send_data(float data, String type) {
  http.begin("http://192.168.1.119:8080/data?" + type + "=" + String(data));
  int httpCode = http.GET(); // Send the request
  if (httpCode != 200) {
    String payload = http.getString(); // Get the request response payload
    Serial.println("REQUEST NOT SUCCESFULLY!");
    Serial.println(payload); // Print the response payload
    flash_n_times(2);
  }
  http.end(); // Close connection
}
