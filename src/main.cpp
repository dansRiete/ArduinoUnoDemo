#include <time.h>
#include <WString.h>
#include <math.h>
#include <HardwareSerial.h>
#include <Time.h>

struct Measure {
    time_t measureTime;
    float pm25;
    float pm10;
    float temp;
    float rh;
    char serviceInfo[10];
};


float calculateAbsoluteHumidity(float temp, float rh) {
    return 6.112 * pow(2.71828, 17.67 * temp / (243.5 + temp)) * rh * 2.1674 / (273.15 + temp);
}


String measureToString(Measure measure) {
    char measureString[100];
    snprintf(measureString, 100, "%02d/%02d/%d %02d:%02d:%02d - PM2.5 = %.1f, PM10 = %.1f, temp = %.1fC(%s), RH = %.0f%%, AH = %.1fg/m3",
             day(measure.measureTime),
             month(measure.measureTime),
             year(measure.measureTime),
             hour(measure.measureTime),
             minute(measure.measureTime),
             second(measure.measureTime),
             measure.pm25,
             measure.pm10,
             measure.temp,
             measure.serviceInfo.c_str(),
             measure.rh,
             measure.temp == -1 || measure.rh == -1 ? -1 : calculateAbsoluteHumidity(measure.temp, measure.rh)
    );
    return String(measureString);
}

void setup() {

    Serial.begin(115200);

    float roofTemp = 30.2;
    float windowTemp= 27.5;
    float roofHumid = 45;
    float windowHumid = 50;
    char serviceInfo[10];

    Measure currentMeasure = {
            now(),
            2.3,
            7.4,
            roofTemp < windowTemp ? roofTemp : windowTemp,
            roofTemp < windowTemp ? roofHumid : windowHumid,
            {static_cast<char>(
                     snprintf(serviceInfo, 10, "%s-%.1fC/%.0f%%",
                              windowTemp < roofTemp ? "W" : "R",
                              windowTemp < roofTemp ? roofTemp : windowTemp,
                              windowTemp < roofTemp ? roofHumid : windowHumid
                     )
             )
            }

    };

    Serial.println(measureToString(currentMeasure));
}

void loop() {}
