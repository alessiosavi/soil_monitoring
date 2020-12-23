#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <time.h>

#define DHTPIN1 12  // D6
#define DHTPIN2 14  // D5
#define SOIL_PIN 4  // D2
#define LIGHT_PIN 5 // D1

const char *ssid = "TIM-29854979";
const char *password = "billgatesfinocchio";

DHT dht[] = {
    {DHTPIN1, DHT11},
    {DHTPIN2, DHT11},
};
WiFiServer server(80);

int analogSoil() {
  delay(100);
  digitalWrite(SOIL_PIN, HIGH);
  delay(100);
  digitalWrite(LIGHT_PIN, LOW);
  delay(100);
  return analogRead(0);
}

int analogLight() {
  delay(100);
  digitalWrite(LIGHT_PIN, HIGH);
  delay(100);
  digitalWrite(SOIL_PIN, LOW);
  delay(100);
  return analogRead(0);
}

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
  pinMode(SOIL_PIN, INPUT);
}

// Declare an object of class HTTPClient
HTTPClient http;

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WIFI DISCONNECTED! Trying to reconnect");
    connect();
  }
  float temp = -1;
  float hum = -1;
  //  int light = -1;
  //  int soil = -1;
  for (auto &sensor : dht) {

    // Read and send temperature
    temp = sensor.readTemperature(false, true);
    send_data(temp, "temp");

    // Read and send the humidity
    hum = sensor.readHumidity(false);
    send_data(hum, "hum");

    //    // Read and send light
    //    light = analogLight();
    //    Serial.println(light);
    //    send_data(light , "light");
    //
    //    // Read and send soil
    //    soil = analogSoil();
    //    Serial.println(soil);
    //    send_data(soil , "soil");
  }
  delay(2000);
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
