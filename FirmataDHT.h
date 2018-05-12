/*
  FirmataDHT.h - Firmata library
  Copyright (C) 2013 Norbert Truchsess. All rights reserved.
  Copyright (C) 2014 Nicolas Panel. All rights reserved.
  Copyright (C) 2015 Jeff Hoefs. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  See file LICENSE.txt for further informations on licensing terms.

*/

#ifndef FirmataDHT_h
#define FirmataDHT_h

#include <ConfigurableFirmata.h>
#include <FirmataFeature.h>

#include <idDHTLib.h>

#define DHT_SENSOR_MINIMUM_UPDATE_INTERVAL 500 // sensor is too slow, so don't update more frequent than this
#define DHT_SENSOR_INIT_TIME 3000 // sensor needs to initialize before first update
#define DHT_SENSOR_RETRY_INTERVAL 1000 // time to wait before retrying after unsuccessful read

#define DHTSENSOR_DETACH        (0x00)
#define DHTSENSOR_ATTACH_DHT11  (0x01)
#define DHTSENSOR_ATTACH_DHT22  (0x00)

class FirmataDHT : public FirmataFeature
{
public:
  FirmataDHT();
  //~FirmataDHT(); => never destroy in practice

  // FirmataFeature implementation
  boolean handlePinMode(byte pin, int mode);
  void handleCapability(byte pin);
  boolean handleSysex(byte command, byte argc, byte *argv);
  void reset();

  // FirmataDHT implementation
  void update();
  boolean isAttached();
  void attachDHTSensor(byte pinNum, idDHTLib::DHTType sensorType, uint32_t samplingInterval = DHT_SENSOR_MINIMUM_UPDATE_INTERVAL);
  void detachDHTSensor();

private:
  // uint8_t m_attachedPin = NOT_A_PIN;
  uint32_t m_samplingInterval = DHT_SENSOR_MINIMUM_UPDATE_INTERVAL;
  unsigned long m_lastUpdateMillis = 0;
  idDHTLib *m_dhtSensor = nullptr;
  bool m_initialized = false;
  bool m_acquiring = false;

  void report();
};

#endif
