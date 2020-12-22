#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h> // Include the Wi-Fi library
#include <time.h>

#define DHTPIN1 14
#define DHTPIN2 12
const char *ssid = "TIM-29854979";
const char *password = "billgatesfinocchio";

DHT dht[] = {
    {DHTPIN1, DHT11},
    {DHTPIN2, DHT11},
};

// DHT dht1(DHTPIN1, DHTTYPE);
// DHT dht2(DHTPIN2, DHTTYPE);
WiFiServer server(80);

void connect() {
  WiFi.begin(ssid, password); // Connect to the network
  Serial.print("Connecting to " + String(ssid) + " ...");
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

}

HTTPClient http; // Declare an object of class HTTPClient
float temp = -1;
float hum = -1;
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WIFI DISCONNECTED! Trying to reconnect");
    connect();
  }

  for (auto &sensor : dht) {
    temp = sensor.readTemperature(false, true);
    send_data(temp, "temp");
    hum = sensor.readHumidity(false);
    send_data(hum, "hum");
  }
  delay(5000);
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
