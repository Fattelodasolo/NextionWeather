#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESPHTTPClient.h>
#include <time.h>
#include <SoftwareSerial.h>
#include "Astronomy.h"
#include <SHT3x.h>

#define UTC_OFFSET 1
#define DST_OFFSET 1
#define NTP_SERVERS "pool.ntp.org"

#define RX_PIN D5
#define TX_PIN D6

const char* ssid = "xxxxxxxxxxxxxx";          // SSID Wi-Fi
const char* password =  "xxxxxxxxxxx";        // Password Wi-Fi


const String city_id = "xxxxxxx";             // ID Città
const String apikey = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";        //API Key
const String units = "metric";

char url[] = "api.openweathermap.org";

const String MOON_PHASES[] = {"Novilunio", "Mezzaluna Crescente", "Primo Quarto", "Gibbosa Crescente",
                              "Plenilunio", "Gibbosa Calante", "Ultimo Quarto", "Mezzaluna Calante"};

String information = "";
String data = "";

unsigned long nowMillis, lastMillis, currentMillis, previousMillis;

Astronomy astronomy;

SHT3x Sensor;
SoftwareSerial myPort;

void setup() {
  Serial.begin(115200);
  myPort.begin(9600, SWSERIAL_8N1, RX_PIN, TX_PIN, false);
  Sensor.Begin();
  
  Serial.println();
  Serial.println();
  Serial.print("Connessione a ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("Connesso");
  Serial.println("");
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());

  configTime(UTC_OFFSET, DST_OFFSET * 60, NTP_SERVERS);
  Serial.println("Sincr. orario...");
  while(time(nullptr) <= 10000) {
    Serial.print(".");
    delay(1);
  }
  Serial.println("");
  Serial.println("Orario sincronizzato");

}

void loop() {

  WiFiClient client;
  
   if (client.connect(url, 80))
  { //avvia la connessione del client
    client.println("GET /data/2.5/weather?id=" + city_id + "&units=" + units + "&APPID=" + apikey);
    client.println("Host: api.openweathermap.org");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();
  }
  else {
    Serial.println("connessione fallita");
    Serial.println();
    return;
  }

   while (client.connected() && !client.available())          // attendo mentre il client si è collegato ed è disponibile
    delay(10);
  while (client.connected() || client.available())            // mentre il client è connesso e disponibile, leggo i dati
  { 
    char reading = client.read();
    information = information + reading;
    delay(10);
  }

  client.stop();                                              // interrompo la connessione del client
  
  // Elaboro i dati ricevuti in una sola volta e li smembro, in modo da ottenere le singole informazioni utili
  information.replace('[', ' ');
  information.replace(']', ' ');
  char jsonArray [information.length() + 1];
  information.toCharArray(jsonArray, sizeof(jsonArray));
  jsonArray[information.length() + 1] = '\0';
  StaticJsonDocument<1024> doc;
  DeserializationError  error = deserializeJson(doc, jsonArray);

  if (error) {
    Serial.print(F("deserializeJson() fallito "));
    Serial.println(error.c_str());
    return;
  }

  String location = doc["name"];            // Città
  String country = doc["sys"]["country"];   // Paese
  int t = doc["main"]["temp"];              // Temperatura
  int h = doc["main"]["humidity"];          // Umidità
  int p = doc["main"]["pressure"];          // Pressione
  String wind = doc["wind"]["speed"];       // Velocità del vento
  String weather = doc["weather"]["main"];  // Condizione meteo
  String icon = doc["weather"]["icon"];     // ID icona

  Serial.println();
  Serial.print("Paese: ");
  Serial.println(country);
  Serial.print("Luogo: ");
  Serial.println(location);
  Serial.print("Temperatura: ");
  Serial.print(t);
  Serial.println("°C");
  Serial.printf("Umidità: ");
  Serial.print(h);
  Serial.println("%");
  Serial.printf("Pressione: ");
  Serial.print(p);
  Serial.println(" mBar");
  Serial.printf("Vento: ");
  Serial.print(wind);
  Serial.println(" m/s");
  Serial.print("Meteo: ");
  Serial.print(weather);
  Serial.print(" ");
  Serial.println (icon);

  time_t now = time(nullptr);
  struct tm * timeinfo = localtime (&now);

  uint8_t phase = astronomy.calculateMoonPhase(timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);
  String MOON = MOON_PHASES[phase].c_str();
  Serial.print("Moon Phase: ");
  Serial.println(MOON);

  Astronomy::MoonData moonData = astronomy.calculateMoonData(timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);
  Serial.printf("Moon Phase: %s\n", MOON_PHASES[moonData.phase].c_str());
  Serial.printf("Illumination: %f\n", moonData.illumination  * 100.00);

  float l = moonData.illumination * 100.00;

//Indicazioni meteo per cambio icona
  nowMillis = millis();
  if ( nowMillis - lastMillis > 500){
      lastMillis = nowMillis;
      
  if (icon == "01d" || icon == "01n"){
      myPort.print("va0.val=");
      myPort.print(1);
      myPort.write(0xff);
      myPort.write(0xff);
      myPort.write(0xff);                          // soleggiato
      }

  else if (icon == "02d" || icon == "02n" || icon == "03d" || icon == "03n"){
      myPort.print("va0.val=");
      myPort.print(2);
      myPort.write(0xff);
      myPort.write(0xff);
      myPort.write(0xff);                          // parzialmente soleggiato-nuvoloso
          
      }

  else if (icon == "04d" || icon == "04n"){
      myPort.print("va0.val=");
      myPort.print(7);
      myPort.write(0xff);
      myPort.write(0xff);
      myPort.write(0xff);                          // nuvoloso
      
      }

  else if (icon == "09d" || icon == "09n" || icon == "10d" || icon == "10n"){
      myPort.print("va0.val=");
      myPort.print(4);
      myPort.write(0xff);
      myPort.write(0xff);
      myPort.write(0xff);                          // piovoso
    
      }

  else if (icon == "11d" || icon == "11n"){
      myPort.print("va0.val=");
      myPort.print(6);
      myPort.write(0xff);
      myPort.write(0xff);
      myPort.write(0xff);                        // temporale
    
      }

  else if (icon == "13d" || icon == "13n"){
      myPort.print("va0.val=");
      myPort.print(5);
      myPort.write(0xff);
      myPort.write(0xff);
      myPort.write(0xff);                        // neve
    
      }

  else if (icon == "50d" || icon == "50n"){
      myPort.print("va0.val=");
      myPort.print(3);
      myPort.write(0xff);
      myPort.write(0xff);
      myPort.write(0xff);                          // nebbia
      
      }

  else {}


  if (MOON == "Novilunio"){
    myPort.print("va2.val=");
    myPort.print(9);
    myPort.write(0xff);
    myPort.write(0xff);
    myPort.write(0xff);              // Novilunio
  }

  else if (MOON == "Mezzaluna Crescente"){
    myPort.print("va2.val=");
    myPort.print(10);
    myPort.write(0xff);
    myPort.write(0xff);
    myPort.write(0xff);              // Mezzaluna Crescente
  }

    else if (MOON == "Primo Quarto"){
    myPort.print("va2.val=");
    myPort.print(11);
    myPort.write(0xff);
    myPort.write(0xff);
    myPort.write(0xff);              // Primo quarto
  }

    else if (MOON == "Gibbosa Crescente"){
    myPort.print("va2.val=");
    myPort.print(12);
    myPort.write(0xff);
    myPort.write(0xff);
    myPort.write(0xff);              // Gibbosa crescente
  }
    
    else if (MOON == "Plenilunio"){
    myPort.print("va2.val=");
    myPort.print(13);
    myPort.write(0xff);
    myPort.write(0xff);
    myPort.write(0xff);              // Plenilunio
  }

    else if (MOON == "Gibbosa Calante"){
    myPort.print("va2.val=");
    myPort.print(14);
    myPort.write(0xff);
    myPort.write(0xff);
    myPort.write(0xff);              // Gibbosa calante
  }

    else if (MOON == "Ultimo Quarto"){
    myPort.print("va2.val=");
    myPort.print(15);
    myPort.write(0xff);
    myPort.write(0xff);
    myPort.write(0xff);              // Ultimo quarto
  }

    else if (MOON == "Mezzaluna Calante"){
    myPort.print("va2.val=");
    myPort.print(16);
    myPort.write(0xff);
    myPort.write(0xff);
    myPort.write(0xff);              // Mezzaluna calante
  }
 }


  currentMillis = millis();
  if ( currentMillis - previousMillis > 5000){
    previousMillis = currentMillis;

    Sensor.UpdateData();

// Invio i dati alla Page 1 del Nextion  
    data = "t0.txt=\"" + String(Sensor.GetTemperature()) + "\"";
      myPort.print(data);
      myPort.write(0xff);
      myPort.write(0xff);
      myPort.write(0xff);

    data = "t1.txt=\"" + String(Sensor.GetRelHumidity()) + "\"";
      myPort.print(data);
      myPort.write(0xff);
      myPort.write(0xff);
      myPort.write(0xff);

    String msg = "Temperatura: " + String(Sensor.GetTemperature(SHT3x::Far)) + " F, Umid. Assoluta: " + String(Sensor.GetAbsHumidity()) + " Torr";

    data = "g0.txt=\"" + msg + "\"";
      myPort.print(data);
      myPort.write(0xff);
      myPort.write(0xff);
      myPort.write(0xff);

    data = "t12.txt=\"" + String(t) + "\"";
      myPort.print(data);
      myPort.write(0xff);
      myPort.write(0xff);
      myPort.write(0xff);
   
// Invio i dati alla Page 2 del Nextion

    data = "g1.txt=\"" + location + ", " + country + "\"";
      myPort.print(data);
      myPort.write(0xff);
      myPort.write(0xff);
      myPort.write(0xff);

    data = "t25.txt=\"" + String(l) + "\"";
      myPort.print(data);
      myPort.write(0xff);
      myPort.write(0xff);
      myPort.write(0xff);


    data = "t15.txt=\"" + String(h) + "\"";
      myPort.print(data);
      myPort.write(0xff);
      myPort.write(0xff);
      myPort.write(0xff);

    data = "t16.txt=\"" + String(p) + "\"";
      myPort.print(data);
      myPort.write(0xff);
      myPort.write(0xff);
      myPort.write(0xff);

    data = "t17.txt=\"" + String(wind) + "\"";
      myPort.print(data);
      myPort.write(0xff);
      myPort.write(0xff);
      myPort.write(0xff);
  }

    information = "";
}
