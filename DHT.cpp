/*
 *  DHT Library for Digital-output Humidity and Temperature sensors
 *
 *  Tested with DHT11, DHT22
 *  Compatible with SEN11301P, SEN51035P, AM2302, HM2303
 *
 *  Copyright (C) Anthony Hinsinger
 *                Inspired from Wim De Roeve MBED library code
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

#define DHT_DATA_LENGTH 40

#define WAIT_PIN_CHANGE(from, timeout, err) timer.reset(); \
    do { \
        if (timer.read_us() > timeout) { \
            return err; \
        } \
    } while (dio == from);

#define MEASURE_PIN_CHANGE(from, elapsed, timeout, err) timer.reset(); \
    do { \
        elapsed = timer.read_us(); \
        if (elapsed > timeout) { \
            return err; \
        } \
    } while (dio == from);

DHT::DHT(PinName pin, DHTFamily family) : _pin(pin), _family(family), _lastReadTime(-1) {
}

DHT::~DHT() {
}

int DHT::read() {
    int i, j, b;
    unsigned int timings[DHT_DATA_LENGTH] = { 0 };
    time_t currentTime;
    DigitalInOut dio(_pin);
    Timer timer;

    currentTime = time(NULL);

    if (_lastReadTime >= 0) {
        if (int(currentTime - _lastReadTime) < 2) {
            return DHT_ERROR_NO_PATIENCE;
        }
    } else {
        _lastReadTime = currentTime;
    }

    timer.start();
    
    // wait bus to be pulled-up
    WAIT_PIN_CHANGE(0, 250, DHT_BUS_BUSY);

    // start signal : low 18ms then release the bus
    dio.output();
    dio = 0;
    wait_ms(18);
    dio = 1;
    dio.input();

    // next steps are timing-dependents
    core_util_critical_section_enter();

    // bus pulled-up 20 to 40us
    WAIT_PIN_CHANGE(1, 60, DHT_ERROR_NOT_PRESENT);

    // sensor start : 80us low + 80us pulled-up
    WAIT_PIN_CHANGE(0, 100, DHT_ERROR_ACK_TOO_LONG);
    WAIT_PIN_CHANGE(1, 100, DHT_ERROR_ACK_TOO_LONG);

    // read data (5x8bits)
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 8; j++) {
            // sensor : 50us low
            WAIT_PIN_CHANGE(0, 100, DHT_ERROR_SYNC_TIMEOUT);
            // sensor : 26-28 (means 0) to 70us (means 1) high
            MEASURE_PIN_CHANGE(1, timings[i*8+j], 100, DHT_ERROR_DATA_TIMEOUT);
        }
    }

    // reading done
    core_util_critical_section_exit();

    timer.stop();

    for (i = 0; i < 5; i++) {
        b=0;
        for (j=0; j<8; j++) {
#ifdef DHTDEBUG
            debug("%d ", timings[i*8+j]);
#endif
            if (timings[i*8+j] >= 38) {
                b |= ( 1 << (7-j));
            }
        }
#ifdef DHTDEBUG
        debug("\r\n");
#endif
        _data[i]=b;
    }

#ifdef DHTDEBUG
    debug("%02x %02x %02x %02x %02x\r\n", _data[0], _data[1], _data[2], _data[3], _data[4]);
#endif

    if (_data[4] == ((_data[0] + _data[1] + _data[2] + _data[3]) & 0xFF)) {
        _lastTemperature = calcTemperature();
        _lastHumidity = calcHumidity();
    } else {
        return DHT_ERROR_CHECKSUM;
    }

    return DHT_ERROR_NONE;

}

int* DHT::getRawData() {
    return _data;
}

float DHT::calcTemperature() {
    int v;

    switch (_family) {
        case DHT_11:
            v = _data[2];
            return float(v);
        case DHT_22:
            v = _data[2] & 0x7F;
            v *= 256;
            v += _data[3];
            if (_data[2] & 0x80)
                v *= -1;
            return float(v)/10;
    }
    return 0;
}

float DHT::calcHumidity() {
    int v;

    switch (_family) {
        case DHT_11:
            v = _data[0];
            return float(v);
        case DHT_22:
            v = _data[0];
            v *= 256;
            v += _data[1];
            return float(v)/10;
    }
    return 0;
}

float DHT::toFarenheit(float celsius) {
    return celsius * 9 / 5 + 32;
}

float DHT::toKelvin(float celsius) {
    return celsius + 273.15;
}

float DHT::getTemperature(DHTScale scale) {
    if (scale == DHT_FARENHEIT)
        return toFarenheit(_lastTemperature);
    else if (scale == DHT_KELVIN)
        return toKelvin(_lastTemperature);
    else
        return _lastTemperature;
}

float DHT::getHumidity() {
    return _lastHumidity;
}
