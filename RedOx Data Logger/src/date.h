#ifndef DATE_H
#define DATE_H

#include <RTClib.h>
#include <Wire.h>


// Fonction pour obtenir et afficher la date et l'heure actuelles
void printDateTime(DateTime& now) {

    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print("\t");
    Serial.print(now.hour(), DEC);
    Serial.print("h");
    Serial.print(now.minute(), DEC);
    Serial.print("m");
    Serial.print(now.second(), DEC);
    Serial.print("s\t");

}

#endif // DATE_H
