#include <Arduino.h>
#include <time.h>
#include "../lib/Time-master/Time.h"
time_t time1 = now();

void setup() {
    Serial.begin(9600);
}

void printTime(time_t time) {
    char buf1[20];
    sprintf(buf1, "%02d:%02d:%02d %02d/%02d/%02d",  hour(time), minute(time), second(time), day(time), month(time), year(time));
    Serial.print(("Date/Time: "));
    Serial.println(buf1);
}

void loop() {
    delay(1000);
//    time_t elapsed = now() - time1;
//    Serial.print(minute(elapsed));
//    Serial.print(":");
//    Serial.println(second(elapsed));
    time_t  customTime1 =  makeTime({0,55,10,0,21,5,0});
    time_t  customTime2 =  makeTime({0,3,0,0,0,0,0});
    Serial.println();
    printTime(now());
    printTime(customTime1);
    printTime(customTime1-now());
}
