//==========================================================
/*    Benjamin Berthelon
      07-2024

  ORP visualiseur & datalogger sur SD card 
  ESP32 

  SD card attached to SPI bus:
  MOSI - pin 23
  MISO - pin 19
  CLK - pin 18
  CS - pin 5

  I2C SSD1306 Oled DISPLAY: 
  SCL = 22  ,  SDA = 21

  RTC module:
  SCL = 22  ,  SDA = 21

  LED rouge = 4
  LED verte = 2

  ORP module: ADC_PIN = 34
  
*/
//==========================================================

//libraries
#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <WebServer.h>
#include "date.h"
#include "index.h"    //HTML pageWEB avec javascripts (acceuil)
#include "graph.h"    //HTML pageWEB avec javascripts (historique avec graphique)
#include "SD.h"
#include "SPI.h"

//Définitons des variables
#define MAISON        0     // Tests avec config maison
#define LED           2     // On board BLUE LED (data sent to Web) et LED verte
#define LED_ERREUR    4     // Led Rouge Erreur & initialisation
#define VOLTAGE       3.30  // ORP module System voltage ESP32 3V3
#define OFFSET        -455  // ORP module Zero drift voltage ATTENTION Bt caliblage SANS SONDE sinon HS
#define ARRAY_LENGTH  200   // Taille du tableau circulaire pour moyenne  (à augmenter si très variable)
#define DELAY_MESURE  25    // Délai entre 2 mesures (defaut = 20 ms)
#define ORP_PIN       34    // ORP module
#define CS_PIN        5     // SD pin CS de la carte SD
#define SCREEN_WIDTH  128   // OLED Affichage largeur en pixels
#define SCREEN_HEIGHT 64    // OLED Affichage hauteur en pixels
#define OLED_ADDR     0x3C  // OLED 0.96" adresse I2C de l'affichage 

// Temps rafraichissement
const long interval_SD = 10000; // SD  intervalle d'enregistrements en millisecondes
const long interval_LCD = 500;  // LCD intervalle d'enregistrements en millisecondes

//Tableau circulaire
int orpArray[ARRAY_LENGTH];
int orpArrayIndex = 0;
int elementsInArray = 0;
unsigned long lastMeasurementTime = 0;

//calcul moyenne ORP
double orpValue;
double averageArray(int *arr, int number) {
    int i;
    long amount = 0;

    for (i = 0; i < number; i++) {
        amount += arr[i];
    }

    return (double)amount / number;
}

WebServer server(80); // Server on port 80
File DataLog;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
RTC_DS1307 rtc;

extern const char GRAPH_page[] PROGMEM;
unsigned long previousMillis_SD = 0;
unsigned long previousMillis_LCD = 0;

#if MAISON
#include <WiFiClient.h>
const char* ssid = "******";
const char* password = "*****";
IPAddress local_IP(192, 168, 0, 66);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
#else
const char* ssid = "Piscine_RedOx";
IPAddress local_IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
#endif

//===============================================================
// This routine is executed when you open its IP in browser
//===============================================================
void handleRoot() {
  String s = MAIN_page;             // Assuming MAIN_page is defined elsewhere
  server.send(200, "text/html", s); // Send web page
}

void handleADC() {
  // fonctionnne qd la page web est ouverte

  int a = orpValue;
  DateTime now = rtc.now();    // Obtenir la date et l'heure actuelles depuis le RTC
  printDateTime(now);          // Appeler la fonction pour afficher la date et l'heure
  Serial.print("Valeur ORP -> HTTP: "); 
  Serial.println(a); 
  String adcValue = String(a);
  digitalWrite(LED, !digitalRead(LED));     // Toggle LED on data request AJAX
  server.send(200, "text/plain", adcValue); // Send ADC value only to client AJAX request
  }

void handleFileDownload() {
  if (server.args() == 0) {
    server.send(500, "text/plain", "BAD ARGS");
    return;
  }
  String path = "/" + server.arg(0); // Ajout du "/" pour rendre le chemin absolu
  File file = SD.open(path);
  if (!file) {
    server.send(404, "text/plain", "File Not Found");
    Serial.println(file);
    return;
  }
  server.streamFile(file, "text/plain"); // Utiliser "text/plain" pour afficher le contenu texte directement
  file.close();
}

void handleListFiles() {
  if (!SD.begin()) {
    server.send(500, "application/json", "[]"); // Erreur si la carte SD n'est pas montée
    return;
  }
  File root = SD.open("/");
  String json = "[";
  bool firstFile = true;
  while (true) {
    File entry = root.openNextFile();
    if (!entry) {
      break;
    }
    if (firstFile) {
      firstFile = false;
    } else {
      json += ",";
    }
    json += "\"" + String(entry.name()) + "\"";
    entry.close();
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleReadFile() {
  if (server.args() == 0) {
    server.send(500, "text/plain", "BAD ARGS");
    return;
  }
  String path = "/" + server.arg("file");
  File file = SD.open(path);
  if (!file) {
    server.send(404, "text/plain", "File Not Found");
    return;
  }
  String fileContent;
  while (file.available()) {
    fileContent += char(file.read());
  }
  file.close();
  server.send(200, "text/plain", fileContent);
}

void handleGraph() {
  server.send_P(200, "text/html", GRAPH_page); // Envoie la page web du graphique
}

void message_initialisation(){
   

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);   
    display.setCursor(0, 0);
    display.println(F("Red/ox"));
    display.println(F("DataLogger"));
    display.setTextSize(1);
    display.println();
    display.println(F("IP: 192.168.1.1"));
    display.println(F("Berthelon.B"));
    display.display();
    delay(4000);
     
 // Afficher message acceuil
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);   
    display.setCursor(0, 0);
    display.println(F("Led Verte:"));
    display.setTextSize(1);
    display.println(F(" 1 enreg./chgt"));
    display.println();
    //display.println(F("---------------------"));
    display.setTextSize(2);
    display.println(F("Led Rouge:"));
    display.setTextSize(1);
    display.println(F(" 1 err/clign."));
    display.display();
    delay(5000);


    // Afficher date
    DateTime now = rtc.now();
    display.setTextColor(SSD1306_WHITE); 
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println(F("  Date:"));
    display.println();
    display.print(now.day(), DEC); display.print(F(":"));
    display.print(now.month(), DEC); display.print(F(":"));
    display.println(now.year(), DEC);
    display.print(now.hour(), DEC); display.print(F("h"));
    display.print(now.minute(), DEC); display.print(F("m"));
    display.print(now.second(), DEC); display.print(F("s"));
    display.display();
    delay(3000);
}

void setup () {

// Initialisation Console serie
  Serial.begin(9600);
  
// Onboard LED port Direction output
pinMode(LED, OUTPUT);
pinMode(LED_ERREUR, OUTPUT);
digitalWrite(LED_ERREUR, HIGH);
int erreur = 0;

#if MAISON == 1
// Wait for connection maison
WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);     //Connect to your WiFi router
Serial.println("");
while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
}
#else

  if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
    Serial.println("AP Config Failed");
    erreur =+ 1; // LED rouge pour erreur
  }

  // Start the AP
  WiFi.softAP(ssid);

#endif

 // If connection successful show IP address in serial monitor
  Serial.println();
  Serial.println("###############");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP()); // IP address assigned to your ESP

  server.on("/", handleRoot);
  server.on("/readADC", handleADC); // This page is called by JavaScript AJAX
  server.on("/readFile", handleReadFile);
  server.on("/listFiles", handleListFiles);
  server.on("/graph", handleGraph);
  server.on("/download", HTTP_GET, handleFileDownload);


  server.begin(); // Start server
  Serial.println("HTTP server started");
  Serial.println("###############");



  // Initialisation de l'horloge
  if (!rtc.begin()) {
    Serial.println("RTC erreur");
    erreur += 1; // Incrémenter le compteur d'erreurs et éventuellement allumer une LED rouge
  } else {
    Serial.println("RTC OK");
  }

  // Vérification si le RTC est à l'heure
  if (!rtc.isrunning()) {
    Serial.println("RTC n'est pas à l'heure");
    // Si le RTC ne fonctionne pas, régler l'heure avec la date et l'heure de compilation du sketch
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // Pour régler une date et une heure spécifiques, vous pouvez utiliser la ligne suivante :
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0)); // Exemple pour le 21 janvier 2014 à 3h00
  } else {
    Serial.println("RTC est à l'heure");
  }

//SD CART
 if (!SD.begin()) {
  delay(100);
    Serial.println("SD Erreur carte non montée");
    erreur += 1; // Incrémenter le compteur d'erreurs et éventuellement allumer une LED rouge
   }
Serial.println("SD Carte montée");


   // Ouvrir ou créer le fichier
  DateTime now = rtc.now();
  char filename[16]; // Format /AAAA-MM.txt
  snprintf(filename, sizeof(filename), "/%04d-%02d.txt", now.year(), now.month());
  DataLog = SD.open(filename, FILE_APPEND);
  if (!DataLog) {
    Serial.println("SD Échec de l'ouverture/création du fichier");
    erreur += 1; // Incrémenter le compteur d'erreurs et éventuellement allumer une LED rouge
  } else {
    Serial.println("SD Ouverture/création du fichier");
  }
 

   
   // Initialisation de l'I2C
    Wire.begin();

    // Initialisation de l'écran OLED
    if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println(F("OLED erreur"));
        erreur =+ 1; // LED rouge pour erreur 
      }
      else {
        Serial.println("OLED OK");
       } 


    message_initialisation();// Afficher message acceuil
    
    
    //================= Consigne LED ERREUR ======================
      if (erreur == 0) {
        digitalWrite(LED_ERREUR, LOW);  // Éteint la LED si erreur est 0
      } else {
        for (int i = 0; i < 5; i++) {  // Répéter 5 fois
          for (int j = 0; j < erreur; j++) {  // Faire clignoter la LED 'erreur' fois
            digitalWrite(LED_ERREUR, HIGH);  // Allume la LED
            delay(100);  // Attendre 100 ms
            digitalWrite(LED_ERREUR, LOW);  // Éteint la LED
            delay(100);  // Attendre 100 ms
          }
          delay(1000 - (erreur * 200));  // Attendre le reste du temps pour compléter 1 seconde
        }
        digitalWrite(LED_ERREUR, HIGH);
                }
      //================= Consigne LED ERREUR ======================    

}

void loop () {
  
// temps écoulé depuis le démarrage du programme
unsigned long currentMillis = millis();

// Mesure ORP si delai dépassé
if (millis() - lastMeasurementTime >= DELAY_MESURE) {
    lastMeasurementTime = millis();

    // Add a new measurement to the circular buffer
    orpArray[orpArrayIndex] = analogRead(ORP_PIN);
    orpArrayIndex = (orpArrayIndex + 1) % ARRAY_LENGTH;

    if (elementsInArray < ARRAY_LENGTH) {
        elementsInArray++;
    }

  }

//Calcul valeur ORP selon le module
orpValue = ((30 * VOLTAGE * 1000) - (75 * averageArray(orpArray, elementsInArray) * VOLTAGE * 1000 / 4095)) / 75 - OFFSET;

// Affichage -> LCD
if (currentMillis - previousMillis_LCD >= interval_LCD) {
  previousMillis_LCD = currentMillis;

  DateTime now = rtc.now();  // Obtenir la date et l'heure actuelles depuis le RTC
  printDateTime(now);  // Appeler la fonction pour afficher la date et l'heure
  
  Serial.print("Valeur ORP -> LCD: "); 
  Serial.print(int (orpValue)); 
  Serial.println(" mv"); 

  // Affichage Valeur
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10,5);
  display.print(F("  Valeur Red/Ox :"));
  display.setTextSize(1);
  
  if (int (orpValue) <= -1000) {
  display.setCursor(0,25);display.setTextSize(4);display.print(int (orpValue));
  }else{
  display.setCursor(116,55);display.print(F("mV"));display.setCursor(0,25);display.setTextSize(5);display.print(int (orpValue));
  }
    
  display.display();
}

// Enregistrement -> SD
if (currentMillis - previousMillis_SD >= interval_SD) {
  previousMillis_SD = currentMillis;

// Obtenir la date et l'heure actuelles
DateTime now = rtc.now();
printDateTime(now);  // Appeler la fonction pour afficher la date et l'heure

int a = orpValue;

// Créer une chaîne avec la date, l'heure et la valeur de a
String logEntry = String(now.year()) + ";" + String(now.month()) + ";" + String(now.day()) + ";" +
            String(now.hour()) + ";" + String(now.minute()) + ";" + String(now.second()) + ";" +
            String(a);

// Écrire l'entrée dans le fichier
if (DataLog) {
DataLog.println(logEntry);
DataLog.flush();  // Assurer que les données sont écrites immédiatement
Serial.print("Valeur ORP -> SD: ");
Serial.println(logEntry);
digitalWrite(LED, !digitalRead(LED)); // Toggle LED 
} else {
Serial.println("Erreur: Impossible d'écrire dans le fichier SD");
}
      
    }

server.handleClient();          //Handle client requests
}
