#include <../lib/Time-master/TimeLib.h>
#include <math.h>
#include <Arduino.h>

#define WORKING_PERIOD 1000
#define SLEEPING_PERIOD 50
const boolean DEBUG = true;
const int SENSOR_RX_PIN = 4;
const int SENSOR_DX_PIN = 5;

struct Measure {
    time_t measureTime;
    int pm25;
    int pm10;
};

Measure nullMeasure = {0, -1, -1};

#define EVERY_MEASURES_NUMBER 60
struct Measure everyMeasures[EVERY_MEASURES_NUMBER];
int everyMeasureIndex = -1;
boolean everyMeasureFirstPass = true;

#define EVERY_15_MINUTES_MEASURES_NUMBER 4*10
struct Measure every15minutesMeasures[EVERY_15_MINUTES_MEASURES_NUMBER];
int every15MinutesMeasureIndex = -1;
boolean every15MinutesMeasureFirstPass = true;
time_t every15minuteTimer;

#define EVERY_HOUR_MEASURES_NUMBER 12
struct Measure everyHourMeasures[EVERY_HOUR_MEASURES_NUMBER];
int everyHourMeasureIndex = -1;
boolean everyHourMeasureFirstPass = true;
time_t everyHourTimer;

boolean thereIsMore = false;
int thereIsMoreCounter = 0;
int totalCounter = 0;
int totalAveragedCounter = 0;

void logAverage(const Measure &measure);

String getTimeString(time_t time) {
    char measureString[10];
    snprintf(measureString, 10, "%02d:%02d:%02d", hour(time), minute(time), second(time));
    return String(measureString);
}

String measureToString(Measure measure) {
    char measureString[60];
    snprintf(measureString, 60, "%dD %02d:%02d:%02d - PM2.5 = %d, PM10 = %d, Total = %d",
             day(measure.measureTime),
             hour(measure.measureTime),
             minute(measure.measureTime),
             second(measure.measureTime),
             measure.pm25,
             measure.pm10,
             measure.pm25 + measure.pm10
    );
    return String(measureString);
}

String measuresToString(boolean html) {
    String measuresString = "";
    int i1 = 0;
    if(!thereIsMore){
        for(int i = EVERY_MEASURES_NUMBER - 1; i >= 0; i--){
            Measure measure = everyMeasures[i];
            time_t currTime = measure.measureTime;
            if (currTime != 0) {
                measuresString += "- " + measureToString(measure);
                if (html) {
                    measuresString += "<br>";
                }
            }
        }
    }
    for (int i = thereIsMore ? thereIsMoreCounter : EVERY_15_MINUTES_MEASURES_NUMBER - 1; i >= 0; i--) {
        if(i1 > 100) {
            thereIsMoreCounter = i;
            break;
        }
        Measure measure = every15minutesMeasures[i];
        time_t currTime = measure.measureTime;
        if (currTime != 0) {
            measuresString += measureToString(measure);
            if (html) {
                measuresString += "<br>";
            }
        }
    }
    thereIsMore = i1 > 100;
    return measuresString;
}

void printAllMeasures() {
    while(thereIsMore) {
        Serial.println(measuresToString(false));
    }
}

void printAllMeasures(Measure measures[], int length) {
    for(int i = 0; i < length; i++){
        if (measures[i].pm10 != -1) {
            Serial.print(getTimeString((measures[i].measureTime)));
            Serial.print(" PM2.5 = ");
            Serial.print(measures[i].pm25);
            Serial.print("; PM10 = ");
            Serial.println(measures[i].pm10);
        }
    }
}

void printMeasure(Measure measure) {
    Serial.println(measureToString(measure));
}

void putEveryMeasure(Measure measure){
    if (++everyMeasureIndex <= EVERY_MEASURES_NUMBER - 1 && everyMeasureFirstPass) {
        everyMeasures[everyMeasureIndex] = measure;
    } else {
        for (int i1 = 1, i2 = 0; i1 < EVERY_MEASURES_NUMBER; i1++, i2++) {
            // Shift all the array's content on one position
            everyMeasures[i2] = everyMeasures[i1];
        }
        everyMeasures[EVERY_MEASURES_NUMBER - 1] = measure;
        if (everyMeasureFirstPass) {
            everyMeasureFirstPass = false;
        }
    }
}

void putEvery15MinuteMeasure(Measure measure){
    if (++every15MinutesMeasureIndex <= EVERY_15_MINUTES_MEASURES_NUMBER - 1 && every15MinutesMeasureFirstPass) {
        every15minutesMeasures[every15MinutesMeasureIndex] = measure;
    } else {
        for (int i1 = 1, i2 = 0; i1 < EVERY_15_MINUTES_MEASURES_NUMBER; i1++, i2++) {
            // Shift all the array's content on one position
            every15minutesMeasures[i2] = every15minutesMeasures[i1];
        }
        every15minutesMeasures[EVERY_15_MINUTES_MEASURES_NUMBER - 1] = measure;
        if (every15MinutesMeasureFirstPass) {
            every15MinutesMeasureFirstPass = false;
        }
    }
}

void putEveryHourMeasure(Measure measure){
    if (++everyHourMeasureIndex <= EVERY_HOUR_MEASURES_NUMBER - 1 && everyHourMeasureFirstPass) {
        everyHourMeasures[everyHourMeasureIndex] = measure;
    } else {
        for (int i1 = 1, i2 = 0; i1 < EVERY_HOUR_MEASURES_NUMBER; i1++, i2++) {
            // Shift all the array's content on one position
            everyHourMeasures[i2] = everyHourMeasures[i1];
        }
        everyHourMeasures[EVERY_HOUR_MEASURES_NUMBER - 1] = measure;
        if (everyHourMeasureFirstPass) {
            everyHourMeasureFirstPass = false;
        }
    }
}

bool anHourElapsed(time_t startTime, time_t endTime) {
    return minute(startTime - endTime) >= 1;
}

bool inAnHourInterval(time_t startTime, time_t endTime) {
    return  year(startTime - endTime) == 1970 &&
            month(startTime - endTime) == 1 &&
            day(startTime - endTime) == 1 &&
            hour(startTime - endTime) == 0 &&
            !anHourElapsed(startTime, endTime);
}

bool fifteenMinutesElapsed(time_t startTime, time_t endTime) {
    return second(startTime - endTime) >= 15;
}

bool inFifteenMinutesInterval(time_t startTime, time_t endTime) {
    return  year(startTime - endTime) == 1970 &&
            month(startTime - endTime) == 1 &&
            day(startTime - endTime) == 1 &&
            hour(startTime - endTime) == 0 &&
            minute(startTime - endTime) == 0 &&
            !fifteenMinutesElapsed(startTime, endTime);
}

Measure calculate15minuteAverage(time_t currentTime) {
    if(DEBUG){
        Serial.println();
        Serial.print(getTimeString(currentTime));
        Serial.print(" - Calculating 15 minutes average ... ");
    }
    double pm25Summ = 0;
    double pm10Summ = 0;
    int counter = 0;
    time_t lastTime = 0;
    for(int i = 0; i < EVERY_MEASURES_NUMBER; i++) {
        Measure measure = everyMeasures[i];
        if(measure.pm25 != -1 && inFifteenMinutesInterval(currentTime, measure.measureTime)){
            if(DEBUG){ logAverage(measure); }
            pm25Summ += measure.pm25;
            pm10Summ += measure.pm10;
            lastTime = measure.measureTime;
            counter++;
            totalAveragedCounter++;
        }
    }
    Measure result;

    if(counter != 0){
        result = { lastTime, (int) round(pm25Summ/counter), (int) round(pm10Summ/counter) };
    } else {
        result = nullMeasure;
    }

    if(DEBUG){
        Serial.println();
        Serial.print("There were ");
        Serial.print(counter);
        Serial.print(" elements averaged.");
        Serial.print(" Total averaged: ");
        Serial.println(totalAveragedCounter);
        Serial.print("Averaged measure: ");
        Serial.println(measureToString(result));
        Serial.println();
    }
    return result;
}

Measure calculate1HourAverage(time_t currentTime) {
    if(DEBUG){
        Serial.println();
        Serial.print(getTimeString(currentTime));
        Serial.print(" - Calculating 1 hour average ... ");
    }
    double pm25Summ = 0;
    double pm10Summ = 0;
    int counter = 0;
    time_t lastTime = 0;
    for(int i = 0; i < EVERY_15_MINUTES_MEASURES_NUMBER; i++) {
        Measure measure = every15minutesMeasures[i];
        if(measure.pm25 != -1 && inAnHourInterval(currentTime, measure.measureTime)){
            if(DEBUG){ logAverage(measure); }
            pm25Summ += measure.pm25;
            pm10Summ += measure.pm10;
            lastTime = measure.measureTime;
            counter++;
        }
    }

    Measure result;

    if(counter != 0){
        result = {lastTime, (int) round(pm25Summ/counter), (int) round(pm10Summ/counter)};
    } else {
        result = nullMeasure;
    }

    if(DEBUG){
        Serial.println();
        Serial.print("There were ");
        Serial.print(counter);
        Serial.println(" elements averaged");
        Serial.print("Averaged measure: ");
        Serial.println(measureToString(result));
        Serial.println();
    }
    return result;
}

void logAverage(const Measure &measure) {
    Serial.print("[");
    Serial.print(getTimeString(measure.measureTime));
    Serial.print(" (");
    Serial.print(measure.pm25);
    Serial.print(", ");
    Serial.print(measure.pm10);
    Serial.print(")], ");
}

void setup() {
    Serial.begin(9600);

    /*server.on("/", []() {
        server.send(200, "text/html", measuresToString(true));
        server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        String content = measuresToString(true);
        Serial.print("content length: ");
        Serial.println(content.length());
        server.send(200, "text/html", content);
        while(thereIsMore){
            content = measuresToString(true);
            server.sendContent(content);
        }
        thereIsMore = false;
        server.client().stop();
    });*/

    randomSeed(42);

    for (auto &reading : everyMeasures) {
        reading = nullMeasure;
    }
    for (auto &reading : every15minutesMeasures) {
        reading = nullMeasure;
    }
    for (auto &reading : everyHourMeasures) {
        reading = nullMeasure;
    }
    delay(500);
    if (DEBUG) { Serial.print(getTimeString(now()));Serial.println(" - The sensor should be woken now");Serial.print("WORKING_PERIOD is ");Serial.println(WORKING_PERIOD);Serial.print("SLEEPING_PERIOD is ");Serial.println(SLEEPING_PERIOD); }
}

void loop() {

    delay(1000);

    if(Serial.available() > 0) {
        int a = Serial.read() - '0';
        if(a == 1){
            printAllMeasures(everyMeasures, EVERY_MEASURES_NUMBER);
        } else if(a == 2){
            printAllMeasures(every15minutesMeasures, EVERY_15_MINUTES_MEASURES_NUMBER);
        } else if(a == 3){
            printAllMeasures(everyHourMeasures, EVERY_HOUR_MEASURES_NUMBER);
        }
    }

    int pm25 = (int) random(0, 50);
    int pm10 = (int) random(0, 50);
    time_t currentTime = now();
    Measure currentMeasure = {currentTime, pm25, pm10};

    if (DEBUG) { Serial.print(++totalCounter); Serial.print(". Got a measure: "); printMeasure(currentMeasure); }


    putEveryMeasure(currentMeasure);

    if (fifteenMinutesElapsed(currentTime, every15minuteTimer)) {
        every15minuteTimer = currentTime;
        putEvery15MinuteMeasure(calculate15minuteAverage(currentTime));
    }

    if (anHourElapsed(currentTime, everyHourTimer)) {
        everyHourTimer = currentTime;
        putEveryHourMeasure(calculate1HourAverage(currentTime));
    }
}
