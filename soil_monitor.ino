#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <time.h>

#define DHTPIN1 13    // D5
#define DHTPIN2 14    // D7
#define ANALOG_PIN A0 // D1

const char *ssid = "TIM-29854979-test";
const char *password = "arduino-test";

DHT dht[] = {
    {DHTPIN1, DHT22},
    {DHTPIN2, DHT22},
};

WiFiClient wificlient;
WiFiServer server(80);
HTTPClient http;

// Number of measure taken before calculate the MEAN
const int MAX_MEASURES = 10;

// Start url in order to verify where the dashboard is available
String dashboard_server = "http://192.168.1.X:8080/status";

int analogRead() { return analogRead(ANALOG_PIN); }
void led_off() { digitalWrite(LED_BUILTIN, HIGH); }
void led_on() { digitalWrite(LED_BUILTIN, LOW); }
void service_discovery();
void flash_n_times(int n);
void connect();
void setup();
void loop();
void send_data(float data, String type);

void connect() {
  Serial.print("Connecting to " + String(ssid) + " ...");
  WiFi.begin(ssid, password); // Connect to the network
  // Turn the LED on by making the voltage LOW
  digitalWrite(LED_BUILTIN, LOW);
  int i = 0;
  // Wait for the Wi-Fi to connect
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(++i);
    Serial.print(' ');
    flash_n_times(1);
  }
  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  // Discover the dashboard server
  // Turn the LED off by making the voltage HIGH
  digitalWrite(LED_BUILTIN, HIGH);
  service_discovery();
}

void setup() {
  Serial.begin(9600);
  // Initialize the LED_BUILTIN pin as an output
  pinMode(LED_BUILTIN, OUTPUT);
  connect();
  Serial.println("DHT BEGIN");
  for (auto &sensor : dht) {
    sensor.begin();
    delay(1000);
  }
  Serial.println("DHT INITIALIZED");
  pinMode(ANALOG_PIN, INPUT);
  wificlient = WiFiClient();
}

void loop() {
  float temp = 0;
  float heat_index = 0;
  float hum = 0;
  int soil = 0;

  float temp_avg;
  float hum_avg;
  float soil_avg;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WIFI DISCONNECTED! Trying to reconnect");
    connect();
  }

  for (auto &sensor : dht) {
    temp += sensor.readTemperature(false, false);
    soil += analogRead();
    hum += sensor.readHumidity(false);
    delay(2100);
  }
  temp_avg = temp / 2.0;
  soil_avg = soil / 2.0;
  hum_avg = hum / 2.0;

  // heat_index = sensor.computeHeatIndex(temp, hum, false);
  heat_index = dht[0].computeHeatIndex(temp_avg, hum_avg, false);
  Serial.print("Humidity: " + String(hum_avg));
  Serial.print(" Temperature: " + String(temp_avg));
  Serial.print(" Heat Index: " + String(heat_index));
  Serial.print(" Soil: " + String(soil_avg));
  Serial.println();

  send_data(temp_avg, "temp");
  send_data(hum_avg, "hum");
  send_data(heat_index, "heat");
  send_data(soil_avg, "soil");
}

void send_data(float data, String type) {
  http.begin(wificlient,
             dashboard_server + "/data?" + type + "=" + String(data));
  int httpCode = http.GET(); // Send the request
  if (httpCode != 200) {
    String payload = http.getString(); // Get the request response payload
    Serial.println("REQUEST NOT SUCCESFULLY!");
    Serial.println(payload); // Print the response payload
    flash_n_times(2);
  }
  http.end(); // Close connection
}

void service_discovery() {
  bool found = false;
  String ip1 = "192";
  String ip2 = "168";
  int ip3 = 1;
  int ip4 = 100;
  String port = "8080";
  //  http.setReuse(true);
  http.setTimeout(500);
  while (!found) {
    Serial.println("Checking the following address: " + dashboard_server);
    http.begin(wificlient, dashboard_server);
    if (http.GET() == 202) {
      String response = http.getString();
      if (response.equals("UP")) {
        found = true;
        break;
      }
    }
    http.end();
    ip4++;
    if (ip3 > 255)
      ip3 = 1;

    if (ip4 > 255) {
      ip4 = 1;
      ip3 += 1;
    }
    dashboard_server = "http://" + ip1 + "." + ip2 + "." + String(ip3) + "." +
                       String(ip4) + ":" + port + "/status";
  }
  dashboard_server.replace("/status", "");
  Serial.println("Found dashboard server: " + dashboard_server);
}

// flash_n_times is delegated to produce N flash from the standard LED
void flash_n_times(int n) {
  led_off();
  for (int i = 0; i < n; i++) {
    led_on();
    delay(50);
    led_off();
    delay(200);
  }
}

void print_array(float data[]) {
  Serial.print('[ ');
  for (int i = 0; i < MAX_MEASURES; i++) {
    Serial.print(data[i]);
    Serial.print(' ');
  }
  Serial.println(']');
}
