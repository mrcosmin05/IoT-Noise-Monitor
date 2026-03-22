#include <WiFi.h>
#include <HTTPClient.h>

// config user
const char* WIFI_SSID = "S22";     // reteaua ta
const char* WIFI_PASS = "treisute";   // parola

// thingspeak cloud 
const long CHANNEL_ID = 3180803;          
const char* API_KEY = "GHDXBB76VYKHDL4N"; 


const int PIN_MICROFON = 36;      
const int PIN_LED_ROSU = 18;     
const int PIN_LED_VERDE = 19;    
const int PIN_BUTON = 0;

// praguri
const int PRAG_ROSU = 40;        // Peste 40 dB - rosu
const int PRAG_VERDE_MIN = 0;   // 0-40 dB - verde

// variabile
float min_dB = 100.0;
float max_dB = 0.0;
double suma_dB = 0.0;
long numar_citiri = 0;
bool inregistrare_activa = false;
int stareButonAnterioara = HIGH;

// functii

int citeste_amplitudine() {
  unsigned long startMillis = millis(); 
  int peakToPeak = 0;
  unsigned int signalMax = 0;
  unsigned int signalMin = 4095;

  while (millis() - startMillis < 50) {
    int sample = analogRead(PIN_MICROFON);
    if (sample < 4095) {  
       if (sample > signalMax) signalMax = sample;
       if (sample < signalMin) signalMin = sample;
    }
  }
  peakToPeak = signalMax - signalMin; 
  return peakToPeak;
}

// amplitudinea in dB 
int converteste_in_dB(int amplitudine) {
  if (amplitudine < 50) {
    return 0;
  }
  int db = map(amplitudine, 50, 4000, 0, 80); 
  if (db < 0) db = 0;
  if (db > 90) db = 90;
  return db;
}


void actualizeaza_leduri(int db) {
  digitalWrite(PIN_LED_ROSU, LOW);
  digitalWrite(PIN_LED_VERDE, LOW);
  
  if (db >= PRAG_ROSU) {
    digitalWrite(PIN_LED_ROSU, HIGH); 
  } 
  else if (db >= PRAG_VERDE_MIN) {
    digitalWrite(PIN_LED_VERDE, HIGH); 
  }
}


// trimite datele la thingspeak
void trimite_cloud(float mn, float mx, float av) {
  if(WiFi.status() != WL_CONNECTED) {
      Serial.println("Wi-Fi pierdut! Reincerc conectarea...");
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      delay(2000);
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://api.thingspeak.com/update?api_key=" + String(API_KEY) + 
                 "&field1=" + String(mn) + "&field2=" + String(mx) + "&field3=" + String(av);
                 
    http.begin(url);
    int httpCode = http.GET();
    if(httpCode > 0) {
      Serial.println("✅ Date trimise la Cloud cu succes!");
      Serial.println("Verifica graficele pe ThingSpeak!");
      digitalWrite(PIN_LED_VERDE, HIGH); delay(2000); digitalWrite(PIN_LED_VERDE, LOW);
    }
    else {
      Serial.print("❌ Eroare trimitere Cloud. Cod: ");
      Serial.println(httpCode);
      digitalWrite(PIN_LED_ROSU, HIGH); delay(2000); digitalWrite(PIN_LED_ROSU, LOW);
    }
    http.end();
  } else {
    Serial.println("❌ Nu s-a putut conecta la Wi-Fi pentru trimitere.");
  }
}

// setup si loop

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_MICROFON, INPUT);
  pinMode(PIN_LED_ROSU, OUTPUT);
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_BUTON, INPUT_PULLUP);
  
  // test led
  digitalWrite(PIN_LED_ROSU, HIGH); delay(300); digitalWrite(PIN_LED_ROSU, LOW);
  digitalWrite(PIN_LED_VERDE, HIGH); delay(300); digitalWrite(PIN_LED_VERDE, LOW);
  
  Serial.println();
  Serial.print("Conectare la Wi-Fi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  int incercari = 0;
  while (WiFi.status() != WL_CONNECTED && incercari < 20) {
    delay(500);
    Serial.print(".");
    incercari++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Conectat la Wi-Fi!");
    Serial.print("IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n⚠️ Nu s-a conectat la Wi-Fi. Verifica Nume/Parola.");
    Serial.println("Sistemul va functiona local (LED-uri), dar nu va trimite date.");
  }
  
  Serial.println("\n--- SISTEM PREGATIT ---");
}

void loop() {
  int stareButon = digitalRead(PIN_BUTON);

  if (stareButon == LOW && stareButonAnterioara == HIGH) {
    delay(50);
    
    if (!inregistrare_activa) {
      min_dB = 100.0; max_dB = 0.0; suma_dB = 0.0; numar_citiri = 0;
      inregistrare_activa = true;
      Serial.println("\n>>> INREGISTRARE INCEPUTA... Vorbeste/Fa zgomot! <<<");
      digitalWrite(PIN_LED_VERDE, HIGH); delay(500); digitalWrite(PIN_LED_VERDE, LOW);
    }
    else {
      if (numar_citiri > 0) {
        float medie = suma_dB / numar_citiri;
        Serial.println("\n>>> STOP INREGISTRARE <<<");
        Serial.printf("Min: %.0f dB | Max: %.0f dB | Medie: %.1f dB\n", min_dB, max_dB, medie);
        
        Serial.println("Se trimite la ThingSpeak...");
        inregistrare_activa = false;
        actualizeaza_leduri(0);
        trimite_cloud(min_dB, max_dB, medie);
      } else {
        Serial.println("\nNu s-au inregistrat date.");
        inregistrare_activa = false;
        actualizeaza_leduri(0);
      }
    }
  }
  stareButonAnterioara = stareButon;

  // masurare
  if (inregistrare_activa) {
    int amp = citeste_amplitudine();
    int db = converteste_in_dB(amp);
    
    actualizeaza_leduri(db);
    
    // stats
    if (db < min_dB) min_dB = db;
    if (db > max_dB) max_dB = db;
    suma_dB += db;
    numar_citiri++;
    
    Serial.printf("dB: %d\n", db);
  }
}