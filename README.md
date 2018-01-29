# mbed-dht

Read DHT11 and DHT22 - or compatible - humidity and temperature sensors.

## Usage

Add the library to your project using mbed CLI (or directly through online IDE):

```# mbed add https://github.com/atoy40/mbed-dht DHT```

## Example

```cpp
#include "mbed.h"
#include "DHT.h"

DHT dht(D8, DHT::DHT22);

int main(void) {
    while(1) {
        wait(3);
        int err = dht.read();
        if (err == DHT::SUCCESS) {
            printf("T = %.1f°C\r\n", dht.getTemperature());
        } else {
            printf("Error code : %d\r\n", err);
        }
        wait(3);
    }
}
```