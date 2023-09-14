/******************************************************************************
https://github.com/sichel94/GUVB-C31SM 

Distributed as-is; no warranty is given.
******************************************************************************/
#include "guvbc31sm.h"

GUVB sensor;

void setup(){
    // start the UART
    Serial.begin(115200);
    Serial.println("--- GUVB-C31SM Demo ---");

    // initialize the UVB Sensor with default paramenters
    if (sensor.begin() == false)
        Serial.println("Cant verify chip ID, check connection?");

    // Sensor ranges
    //sensor.setRange(GUVB_RANGE_X1);
    //sensor.setRange(GUVB_RANGE_X2);
    //sensor.setRange(GUVB_RANGE_X4);
    //sensor.setRange(GUVB_RANGE_X8);
    //sensor.setRange(GUVB_RANGE_X16);
    sensor.setRange(GUVB_RANGE_X32);
    //sensor.setRange(GUVB_RANGE_X64);
    //sensor.setRange(GUVB_RANGE_X128);

    // Sensor integration times
    //sensor.setIntegrationTime(GUVB_INT_100MS);
    //sensor.setIntegrationTime(GUVB_INT_200MS);
    sensor.setIntegrationTime(GUVB_INT_400MS);
    //sensor.setIntegrationTime(GUVB_INT_800MS);
}

void loop(){

    // read the values from the device
    float uvi = sensor.getUVIndex();

    uvi /= 32; // Range correction
    uvi /= 4;  // Integration time correction

    // print the data
    Serial.print("UV Index: ");
    Serial.println(uvi, 2);

    delay(400);
}