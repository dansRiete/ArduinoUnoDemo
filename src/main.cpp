#include <../lib/Time-master/TimeLib.h>
#include <math.h>
#include <Arduino.h>

#define WORKING_PERIOD 1000
#define SLEEPING_PERIOD 50
const boolean DEBUG = true;
const int SENSOR_RX_PIN = 4;
const int SENSOR_DX_PIN = 5;

#define EVERY_MINUTE_MEASURES_NUMBER 15
int everyMinuteMeasureIndex = -1;
boolean everyMinuteMeasureFirstPass = true;

#define EVERY_15_MINUTES_MEASURES_NUMBER 4*3
int every15MinutesMeasureIndex = -1;
boolean every15MinutesMeasureFirstPass = true;

#define EVERY_HOUR_MEASURES_NUMBER 12
int everyHourMeasureIndex = -1;
boolean everyHourMeasureFirstPass = true;

int currentLastReadingIndex = -1;
int currentReadingIndex = -1;
int step = 1;
time_t every15minuteTimer;
time_t everyHourTimer;
int currentHour;
unsigned long currentTimeMillis = 0;
boolean firstPass = true;
boolean thereIsMore = false;
int thereIsMoreCounter = 0;

struct Measure {
    time_t measureTime;
    int pm25;
    int pm10;
};

struct Measure every15minutesMeasures[EVERY_15_MINUTES_MEASURES_NUMBER];
struct Measure everyMinuteMeasures[EVERY_MINUTE_MEASURES_NUMBER];
struct Measure everyHourMeasures[EVERY_HOUR_MEASURES_NUMBER];

String getTimeString(time_t time) {
    char measureString[10];
    snprintf(measureString, 10, "%02d:%02d:%02d", hour(time), minute(time), second(time));
    return String(measureString);
}

String measureToString(Measure measure) {
    char measureString[60];
    snprintf(measureString, 60, "%dD %02d:%02d:%02d - PM2.5 = %*.*s, PM10 = %*.*s, Total = %*.*s\n",
             day(measure.measureTime),
             hour(measure.measureTime),
             minute(measure.measureTime),
             second(measure.measureTime),
             4, 4, String(measure.pm25).c_str(),
             4, 4, String(measure.pm10).c_str(),
             4, 4, String(measure.pm25 + measure.pm10).c_str()
    );
    return String(measureString);
}

String measuresToString(boolean html) {
    String measuresString = "";
    int i1 = 0;
    if(!thereIsMore){
        for(int i = EVERY_MINUTE_MEASURES_NUMBER - 1; i >= 0; i--){
            Measure measure = everyMinuteMeasures[i];
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

void printMeasure(Measure measure) {
    Serial.println(measureToString(measure));
}

void putEveryMinuteMeasure(Measure measure){
    if (++currentReadingIndex <= EVERY_MINUTE_MEASURES_NUMBER - 1 && everyMinuteMeasureFirstPass) {
        everyMinuteMeasures[everyMinuteMeasureIndex] = measure;
    } else {
        for (int i1 = 1, i2 = 0; i1 < EVERY_MINUTE_MEASURES_NUMBER; i1++, i2++) {
            // Shift all the array's content on one position
            everyMinuteMeasures[i2] = everyMinuteMeasures[i1];
        }
        everyMinuteMeasures[EVERY_MINUTE_MEASURES_NUMBER - 1] = measure;
        if (everyMinuteMeasureFirstPass) {
            everyMinuteMeasureFirstPass = false;
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

Measure calculate15minuteAverage() {
    double pm25Summ = 0;
    double pm10Summ = 0;
    int counter = 0;
    time_t lastTime = 0;
    for(int i = 0; i < EVERY_MINUTE_MEASURES_NUMBER; i++) {
        Measure measure = everyMinuteMeasures[i];
        if(minute(now() - measure.measureTime) < 15){
            pm25Summ += measure.pm25;
            pm10Summ += measure.pm10;
            lastTime = measure.measureTime;
            counter++;
        }
    }
    if(counter != 0){
        return {lastTime, (int) round(pm25Summ/counter), (int) round(pm10Summ/counter)};
    } else{
        return {0,0,0};
    }
}

Measure calculate1HourAverage() {
    double pm25Summ = 0;
    double pm10Summ = 0;
    int counter = 0;
    time_t lastTime = 0;
    for(int i = 0; i < EVERY_15_MINUTES_MEASURES_NUMBER; i++) {
        Measure measure = every15minutesMeasures[i];
        if(hour(now() - measure.measureTime) < 1){
            pm25Summ += measure.pm25;
            pm10Summ += measure.pm10;
            lastTime = measure.measureTime;
            counter++;
        }
    }
    if(counter != 0){
        return {lastTime, (int) round(pm25Summ/counter), (int) round(pm10Summ/counter)};
    } else{
        return {0,0,0};
    }
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

    for (auto &reading : every15minutesMeasures) {
        reading = (Measure) {0, -1, -1};
    }
    for (auto &reading : everyMinuteMeasures) {
        reading = (Measure) {0, -1, -1};
    }
    currentTimeMillis = millis();
    if (DEBUG) {
        Serial.print(getTimeString(now()));
        Serial.println(" - The sensor should be woken now");
        Serial.print("WORKING_PERIOD is ");
        Serial.println(WORKING_PERIOD);
        Serial.print("SLEEPING_PERIOD is ");
        Serial.println(SLEEPING_PERIOD);
    }
}

void loop() {
    delay(1000);
    int pm25 = (int) random(0, 50);
    int pm10 = (int) random(0, 50);

    Serial.print("pm25=");
    Serial.print(pm25);
    Serial.print(", pm10=");
    Serial.println(pm10);

    Measure currentMeasure = {now(), pm25, pm10};
    /*putEveryMinuteMeasure(currentMeasure);
    if (minute(now() - every15minuteTimer) >= 15) {
        every15minuteTimer = now();
        putEvery15MinuteMeasure(calculate15minuteAverage());
    }
    if (hour(now() - everyHourTimer) >= 1) {
        everyHourTimer = now();
        putEveryHourMeasure(calculate1HourAverage());
    }*/
    printMeasure(currentMeasure);
}
