# FirmataDHT

A Firmata wrapper for connectig to a DHT Sensor. To be used with [ConfigurableFirmata](https://github.com/firmata/ConfigurableFirmata).

This library adds support for DHT11 and DHT22 sensors to ConfigurableFirmata. It wraps the nice [idDHT](https://github.com/niesteszeck/idDHTLib) library, that uses interrupts to communicate with the DHT sensor and reduce blocking (it's a very slow sensor).

The sensor must be connected to interrupt pins for the library to work.

This has been tested with AVR Arduinos only (Uno, Nano, Duemilanove).

## Installation

1. Make sure you have the following dependencies installed in your `/Arduino/libraries/` folder:
  - [ConfigurableFirmata](https://github.com/firmata/ConfigurableFirmata)
  - [idDHT](https://github.com/niesteszeck/idDHTLib)

2. Clone or download and copy FirmataDHT into your `/Arduino/libraries/` folder.
