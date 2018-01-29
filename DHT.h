/* 
 *  DHT Library for  DHT Humidity and Temperature sensors 
 *  
 *  Tested with DHT11, DHT22
 *
 *  Copyright (C) Anthony Hinsinger
 *                Inspired from Wim De Roeve code
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

#ifndef MBED_DHT_H
#define MBED_DHT_H

#include "mbed.h"

enum eType {
    DHT11     = 11,
    SEN11301P = 11,
    RHT01     = 11,
    DHT22     = 22,
    AM2302    = 22,
    SEN51035P = 22,
    RHT02     = 22,
    RHT03     = 22
} ;

enum eError {
    ERROR_NONE = 0,
    BUS_BUSY = 1,
    ERROR_NOT_PRESENT = 2,
    ERROR_ACK_TOO_LONG = 3,
    ERROR_SYNC_TIMEOUT = 4,
    ERROR_DATA_TIMEOUT = 5,
    ERROR_CHECKSUM = 6,
    ERROR_NO_PATIENCE = 7
} ;

typedef enum {
    CELCIUS = 0,
    FARENHEIT = 1,
    KELVIN = 2
} eScale;


class DHT {

public:
    DHT(PinName pin, int DHTtype);
    ~DHT();
    int read(void);
    int* getRawData();
    float getHumidity(void);
    float getTemperature(eScale Scale);

private:
    PinName _pin;
    int _DHTtype;
    time_t  _lastReadTime;
    float _lastTemperature;
    float _lastHumidity;
    int DHT_data[6];
    float calcTemperature();
    float calcHumidity();
    float toFarenheit(float);
    float toKelvin(float);

};

#endif
