#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_NeoPixel.h>

#define PIN_NEOPIXEL 48  // Ja nestrādā, nomaini uz 38
#define NUM_PIXELS    1  

Adafruit_NeoPixel pixel(NUM_PIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// --- WIFI IESTATĪJUMI ---
const char* ssid     = "wifi_SSID"; 
const char* password = "wifi_PASS";    

// --- n8n WEBHOOK URL ---
const char* n8n_url = "https://N8N/Webhook/12213213123";

unsigned long pedejaisMers = 0;
const long intervals = 5000; // Cik bieži prasīt datus (milisekundēs) -> 5 sekundes

void setup() {
  Serial.begin(115200);
  pixel.begin();
  pixel.setBrightness(50);

  // Pieslēgšanās WiFi
  WiFi.begin(ssid, password);
  Serial.print("Pieslēdzos WiFi...");
  
  while (WiFi.status() != WL_CONNECTED) {
    pixel.setPixelColor(0, pixel.Color(255, 150, 0)); // Dzeltena mirgošana
    pixel.show();
    delay(250);
    pixel.setPixelColor(0, pixel.Color(0, 0, 0));
    pixel.show();
    delay(250);
  }
  
  Serial.println("\nWiFi pieslēgts!");
  pixel.setPixelColor(0, pixel.Color(0, 0, 0)); 
  pixel.show();
}

void loop() {
  unsigned long pasreizējaisLaiks = millis();

  // Izpildām pieprasījumu katras 5 sekundes
  if (pasreizējaisLaiks - pedejaisMers >= intervals) {
    pedejaisMers = pasreizējaisLaiks;

    // Pārbaudām, vai joprojām ir WiFi savienojums
    if (WiFi.status() == WL_CONNECTED) {
      
      // Indikācija: ZILA krāsa nozīmē, ka ESP32 pašlaik nosūta pieprasījumu un gaida atbildi
      pixel.setPixelColor(0, pixel.Color(0, 0, 255));
      pixel.show();

      HTTPClient http;
      
      // Sākam HTTP savienojumu
      http.begin(n8n_url);
      
      Serial.println("[HTTP] Sūtu GET pieprasījumu uz n8n...");
      int httpCode = http.GET(); // Veicam pieprasījumu

      if (httpCode > 0) {
        Serial.printf("[HTTP] Kods: %d\n", httpCode);

        // Ja n8n atbild veiksmīgi (HTTP 200)
        if (httpCode == HTTP_CODE_OK) {
          String atbilde = http.getString();
          Serial.println("Atbilde no n8n: " + atbilde);

          // 🟢 Ja tekstā atrodas '"result": 0' vai '"result":0' -> Iedegas ZAĻŠ
          if (atbilde.indexOf("\"result\": 0") >= 0 || atbilde.indexOf("\"result\":0") >= 0) {
            pixel.setPixelColor(0, pixel.Color(0, 255, 0));
          } 
          // 🔴 Ja tekstā atrodas '"result": 1' vai '"result":1' -> Iedegas SARKANS
          else if (atbilde.indexOf("\"result\": 1") >= 0 || atbilde.indexOf("\"result\":1") >= 0) {
            pixel.setPixelColor(0, pixel.Color(255, 0, 0));
          } 
          // Ja saņemts JSON, bet "result" vērtība nav ne 0, ne 1 -> Vāji balts
          else {
            pixel.setPixelColor(0, pixel.Color(50, 50, 50)); 
          }
          pixel.show();
        }
      } else {
        Serial.printf("[HTTP] Kļūda: %s\n", http.errorToString(httpCode).c_str());
        // Violeta krāsa signalizē par tīkla kļūdu
        pixel.setPixelColor(0, pixel.Color(255, 0, 255));
        pixel.show();
      }
      
      http.end(); // Noslēdzam savienojumu
    } else {
      Serial.println("WiFi pazaudēts, mēģinu pārslēgties...");
      WiFi.begin(ssid, password);
    }
  }
}
