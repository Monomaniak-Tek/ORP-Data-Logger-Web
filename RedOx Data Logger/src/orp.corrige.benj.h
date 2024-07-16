#include <Wire.h>
#include <Arduino.h>

// Define constants
#define VOLTAGE 3.30 // System voltage
#define OFFSET 0 // Zero drift voltage
#define LED 2 // LED pin

double orpValue;

//temps pour une mesure ~ ARRAY_LENGTH * DELAY_MESURE
//tableau circulaire -nouvelle valeur supprime la dernière-
#define ARRAY_LENGTH 200 // Taille du tableau pour moyenne  (à augmenter si très variable)
#define DELAY_MESURE 25 // Délai entre 2 masures (defaut = 20 ms)
#define ORP_PIN 34 // ORP meter, connect to the ADC pin of the Arduino controller

int orpArray[ARRAY_LENGTH];
int orpArrayIndex = 0;
int elementsInArray = 0;
unsigned long lastMeasurementTime = 0; // Variable to track the time of the last measurement

double averageArray(int *arr, int number) {
    int i;
    long amount = 0;

    for (i = 0; i < number; i++) {
        amount += arr[i];
    }

    return (double)amount / number;
}

void setup() {
    Serial.begin(9600);
    pinMode(LED, OUTPUT);
}

void loop() {
    static unsigned long printTime = millis();

    // Check if it's time to take a new measurement
    if (millis() - lastMeasurementTime >= DELAY_MESURE) {
        lastMeasurementTime = millis();

        // Add a new measurement to the circular buffer
        orpArray[orpArrayIndex] = analogRead(ORP_PIN);
        orpArrayIndex = (orpArrayIndex + 1) % ARRAY_LENGTH;

        if (elementsInArray < ARRAY_LENGTH) {
            elementsInArray++;
        }

     }

    if (millis() >= printTime) { // Print a value every 1200 milliseconds and toggle the LED

        // Calcul de l'ORP avec les dernières valeurs du tableau
        // Convert the analog value to ORP according to the circuit
        orpValue = ((30 * VOLTAGE * 1000) - (75 * averageArray(orpArray, elementsInArray) * VOLTAGE * 1000 / 4095)) / 75 - OFFSET; 

        printTime = millis() + 1200;
        
        Serial.print("ORP: ");Serial.print((int)orpValue);Serial.print(" mV\t");Serial.print("AnalogRead: ");
        int c = analogRead(ORP_PIN);Serial.print(c);Serial.print(" / 4095");Serial.print("\t");
        Serial.print("Elements dans le tableau utilisés pour la moyenne: ");Serial.println(elementsInArray);
        
        digitalWrite(LED, 1 - digitalRead(LED));
    }
}
