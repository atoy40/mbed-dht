/*
 *  DHT Library for  Digital-output Humidity and Temperature sensors
 *
 *  Works with DHT11, DHT22
 *             SEN11301P,  Grove - Temperature&Humidity Sensor     (Seeed Studio)
 *             SEN51035P,  Grove - Temperature&Humidity Sensor Pro (Seeed Studio)
 *             AM2302   ,  temperature-humidity sensor
 *             HM2303   ,  Digital-output humidity and temperature sensor
 *
 *  Copyright (C) Anthony Hinsinger
 *                Started from Wim De Roeve code
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "DHT.h"

#define DHT_DATA_BIT_COUNT 41

#define WAIT_PIN_CHANGE(from, timeout, err) timer.reset(); \
    do { \
        if (timer.read_us() > timeout) { \
            return err; \
        } \
    } while (DHT_io==from);

#define MEASURE_PIN_CHANGE(from, elapsed, timeout, err) timer.reset(); \
    do { \
        elapsed = timer.read_us(); \
        if (elapsed > timeout) { \
            return err; \
        } \
    } while (DHT_io==from);

DHT::DHT(PinName pin,int DHTtype) : _pin(pin), _firsttime(true), _DHTtype(DHTtype) {
}

DHT::~DHT() {
}

int DHT::readData() {
    int i, j, b;
    unsigned int bitTimes[DHT_DATA_BIT_COUNT] = {0};
    time_t currentTime = time(NULL);
    DigitalInOut DHT_io(_pin);
    Timer timer;

    if (!_firsttime) {
        if (int(currentTime - _lastReadTime) < 2) {
            return ERROR_NO_PATIENCE;
        }
    } else {
        _firsttime=false;
        _lastReadTime=currentTime;
    }

    timer.start();
    core_util_critical_section_enter();
    
    // wait bus to be pull-up
    WAIT_PIN_CHANGE(0, 250, BUS_BUSY);

    // start signal : low 18ms + pull-up 20 to 40us
    DHT_io.output();
    DHT_io = 0;
    wait_ms(18);
    DHT_io = 1;
    DHT_io.input();
    WAIT_PIN_CHANGE(1, 60, ERROR_NOT_PRESENT);

    // sensor start : 80us low + 80us pull-up
    WAIT_PIN_CHANGE(0, 100, ERROR_ACK_TOO_LONG);
    WAIT_PIN_CHANGE(1, 100, ERROR_ACK_TOO_LONG);

    // read data (5x8bits)
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 8; j++) {
            // sensor : 50us low
            WAIT_PIN_CHANGE(0, 100, ERROR_SYNC_TIMEOUT);
            // sensor : 26-28 (means 0) to 70us (means 1) pull-up
            MEASURE_PIN_CHANGE(1, bitTimes[i*8+j], 100, ERROR_DATA_TIMEOUT);
        }
    }

    core_util_critical_section_exit();
    timer.stop();

    for (i = 0; i < 5; i++) {
        b=0;
        for (j=0; j<8; j++) {
            debug("%d ", bitTimes[i*8+j]);
            if (bitTimes[i*8+j] >= 38) {
                b |= ( 1 << (7-j));
            }
        }
        debug("\r\n");
        DHT_data[i]=b;
    }

    debug("%02x %02x %02x %02x %02x\r\n", DHT_data[0], DHT_data[1], DHT_data[2], DHT_data[3], DHT_data[4]);

    if (DHT_data[4] == ((DHT_data[0] + DHT_data[1] + DHT_data[2] + DHT_data[3]) & 0xFF)) {
        _lastTemperature=CalcTemperature();
        _lastHumidity=CalcHumidity();
    } else {
        return ERROR_CHECKSUM;
    }

    return ERROR_NONE;

}

int* DHT::rawData() {
    return DHT_data;
}

float DHT::CalcTemperature() {
    int v;

    switch (_DHTtype) {
        case DHT11:
            v = DHT_data[2];
            return float(v);
        case DHT22:
            v = DHT_data[2] & 0x7F;
            v *= 256;
            v += DHT_data[3];
            if (DHT_data[2] & 0x80)
                v *= -1;
            return float(v)/10;
    }
    return 0;
}

float DHT::ReadHumidity() {
    return _lastHumidity;
}

float DHT::ConvertCelciustoFarenheit(float celsius) {
    return celsius * 9 / 5 + 32;
}

float DHT::ConvertCelciustoKelvin(float celsius) {
    return celsius + 273.15;
}

// dewPoint function NOAA
// reference: http://wahiduddin.net/calc/density_algorithms.htm
float DHT::CalcdewPoint(float celsius, float humidity) {
    float A0= 373.15/(273.15 + celsius);
    float SUM = -7.90298 * (A0-1);
    SUM += 5.02808 * log10(A0);
    SUM += -1.3816e-7 * (pow(10, (11.344*(1-1/A0)))-1) ;
    SUM += 8.1328e-3 * (pow(10,(-3.49149*(A0-1)))-1) ;
    SUM += log10(1013.246);
    float VP = pow(10, SUM-3) * humidity;
    float T = log(VP/0.61078);   // temp var
    return (241.88 * T) / (17.558-T);
}

// delta max = 0.6544 wrt dewPoint()
// 5x faster than dewPoint()
// reference: http://en.wikipedia.org/wiki/Dew_point
float DHT::CalcdewPointFast(float celsius, float humidity)
{
        float a = 17.271;
        float b = 237.7;
        float temp = (a * celsius) / (b + celsius) + log(humidity/100);
        float Td = (b * temp) / (a - temp);
        return Td;
}

float DHT::ReadTemperature(eScale Scale) {
    if (Scale == FARENHEIT)
        return ConvertCelciustoFarenheit(_lastTemperature);
    else if (Scale == KELVIN)
        return ConvertCelciustoKelvin(_lastTemperature);
    else
        return _lastTemperature;
}

float DHT::CalcHumidity() {
    int v;

    switch (_DHTtype) {
        case DHT11:
            v = DHT_data[0];
            return float(v);
        case DHT22:
            v = DHT_data[0];
            v *= 256;
            v += DHT_data[1];
            v /= 10;
            return float(v);
    }
    return 0;
}


