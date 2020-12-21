#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h> // Include the Wi-Fi library
#include <time.h>

#define DHTPIN 14
#define DHTTYPE DHT11
const char *ssid = "TIM-29854979";
const char *password = "billgatesfinocchio";

DHT dht(DHTPIN, DHTTYPE);
WiFiServer server(80);

void connect() {
  WiFi.begin(ssid, password); // Connect to the network
  Serial.print("Connecting to " + String(ssid) + " ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(10000);
    Serial.print(++i);
    Serial.print(' ');
  }

  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  // Send the IP address of the ESP8266 to the computer
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(9600);
  connect();
  Serial.println("DHT BEGIN");
  dht.begin();
}

HTTPClient http; // Declare an object of class HTTPClient

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WIFI DISCONNECTED! Trying to reconnect");
    connect();
  }
  Serial.println("Retrieving temp/humidity");
  float temperature = dht.readTemperature(false, true);
  float humidity = dht.readHumidity(false);
  Serial.print("Temp: ");
  Serial.println(temperature);
  send_data(temperature, "temp");
  Serial.print("Humidity: ");
  Serial.println(humidity);
  send_data(humidity, "hum");
  delay(4500);
}

void send_data(float data, String type) {
  http.begin("http://192.168.1.119:8080/data?" + type + "=" + String(data));
  int httpCode = http.GET(); // Send the request
  if (httpCode != 200) {
    String payload = http.getString(); // Get the request response payload
    Serial.println("REQUEST NOT SUCCESFULLY!");
    Serial.println(payload); // Print the response payload
  }
  http.end(); // Close connection
}
