#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <time.h>

#define VIN 5         // V power voltage
#define R 10000       // ohm resistance value
#define DHTPIN1 12    // D6
#define DHTPIN2 14    // D5
#define ANALOG_PIN A0 // D1

const char *ssid = "TIM-29854979";
const char *password = "billgatesfinocchio";

DHT dht[] = {
    {DHTPIN1, DHT22},
    {DHTPIN2, DHT22},
};

WiFiClient wificlient;
WiFiServer server(80);
HTTPClient http;

// Number of measure taken before calculate the MEAN
const int MAX_MEASURES = 10;

float temps[10];
float hums[10];
float heats[10];

// Start url in order to verify where the dashboard is available
String dashboard_server = "http://192.168.1.100:8080/status";

int analogRead() { return analogRead(ANALOG_PIN); }
void led_off() { digitalWrite(LED_BUILTIN, HIGH); }
void led_on() { digitalWrite(LED_BUILTIN, LOW); }
void service_discovery();
void flash_n_times(int n);
void connect();
void setup();
void loop();
int analogToLumen(int raw);
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
    delay(500);
  }
  Serial.println("DHT INITIALIZED");
  pinMode(ANALOG_PIN, INPUT);
  wificlient = WiFiClient();
}

void loop() {
  float temp = -1;
  float heat_index = -1;
  float hum = -1;

  int light = -1;
  int lumen = -1;

  int ti = 0;
  int th = 0;
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WIFI DISCONNECTED! Trying to reconnect");
    connect();
  }

  while (ti < MAX_MEASURES || th < MAX_MEASURES) {
    for (auto &sensor : dht) {
      if (ti < MAX_MEASURES) {
        delay(250);
        temp = sensor.readTemperature(false, true);
        // Serial.println("TEMP: " + String(temp));
        if (isnan(temp) || temp < 1 || temp > 40)
          //        if (isnan(temp))
          flash_n_times(2);
        else {
          // send_data(temp, "temp");
          temps[ti] = temp;
          ti++;
        }
      }

      if (th < MAX_MEASURES) {
        delay(250);
        hum = sensor.readHumidity(false);
        //        Serial.println("HUM: " + String(hum));
        if (isnan(hum) || hum < 1 || hum > 100)
          //        if (isnan(hum))
          flash_n_times(3);
        else {
          // send_data(hum, "hum");
          hums[th] = hum;
          th++;
        }
      }
    }
  }
  temp, hum, heat_index = 0;
  //  Serial.println("TEMPS: ");
  //  print_array(temps);
  //  Serial.println("HUMIDITY: ");
  //  print_array(hums);
  //  Serial.println("HEAT INDEXES: ");
  //  print_array(heats);
  for (int i = 0; i < MAX_MEASURES; i++) {
    temp += temps[i];
    temps[i] = 0;

    hum += hums[i];
    hums[i] = 0;

    // heat_index += heats[i];
    // heats[i] = 0;
  }

  temp = temp / float(MAX_MEASURES);
  hum = hum / float(MAX_MEASURES);

  // delay(300);
  heat_index = dht[0].computeHeatIndex(temp, hum, false);
  // //        Serial.println("HEAT: " + String(heat_index));
  // if (isnan(heat_index) || heat_index < 1 || heat_index > 50)
  //   flash_n_times(4);
  // else {
  //   // send_data(heat_index, "heat");
  //   heats[the] = heat_index;
  //   the++;
  // }

  Serial.print("Humidity: " + String(hum));
  Serial.print(" Temperature: " + String(temp));
  Serial.print(" Heat Index: " + String(heat_index));
  Serial.println();
  send_data(temp, "temp");
  send_data(hum, "hum");
  send_data(heat_index, "heat");

  // Read and send light
  light = analogRead();
  Serial.println("Light: " + String(light));
  send_data(float(light), "light");
  lumen = analogToLumen(light);
  Serial.println("Lumen: " + String(lumen));
  send_data(float(lumen), "lumen");
  //
  //    // Read and send soil
  //    soil = analogSoil();
  //    Serial.println(soil);
  //    send_data(soil , "soil");
  delay(1000);
}

int analogToLumen(int raw) {
  // Conversion rule
  float Vout = float(raw) * (VIN / float(1023)); // Conversion analog to voltage
  float RLDR = (R * (VIN - Vout)) / Vout; // Conversion voltage to resistance
  int phys = 500 / (RLDR / 1000);         // Conversion resitance to lumen
  return phys;
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
  int ip4 = 2;
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

void print_array(float data[]) {
  Serial.print('[ ');
  for (int i = 0; i < MAX_MEASURES; i++) {
    Serial.print(data[i]);
    Serial.print(' ');
  }
  Serial.println(']');
}
