/*
  FirmataDHT.cpp - Firmata library v0.1.0 - 2015-11-22
  Copyright (C) 2013 Norbert Truchsess. All rights reserved.
  Copyright (C) 2014 Nicolas Panel. All rights reserved.
  Copyright (C) 2015 Jeff Hoefs. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  See file LICENSE.txt for further informations on licensing terms.

  Provide encoder feature based on idDHTLib library.
  See https://github.com/niesteszeck/idDHTLib for more informations
*/

// #include <ConfigurableFirmata.h>
// #include <string.h>
#include "FirmataDHT.h"
#include <idDHTLib.h>


/* Constructor */
FirmataDHT::FirmataDHT()
{
}

void FirmataDHT::attachDHTSensor(byte pinNum, idDHTLib::DHTType sensorType, bool blockingReads, uint32_t samplingInterval /* = DHT_SENSOR_MINIMUM_UPDATE_INTERVAL */)
{
  if (isAttached())
  {
    Firmata.sendString("DHT Warning: sensor already attached.");
    return;
  }

  if (!IS_PIN_INTERRUPT(pinNum))
  {
    Firmata.sendString("DHT Error: Can only be used with Interrput pins.");
    return;
  }

  m_blockingReads = blockingReads;
  m_samplingInterval = max(samplingInterval, DHT_SENSOR_MINIMUM_UPDATE_INTERVAL);
  // The sensor needs ~2 sec to initialize, so no response before this call
  // TODO: make sure update is not requested before initialization time
  m_lastUpdateMillis = millis() + DHT_SENSOR_INIT_TIME - m_samplingInterval;;

  Firmata.setPinMode(pinNum, PIN_MODE_DHT);
  m_dhtSensor = new idDHTLib(pinNum, sensorType);
  // Firmata.sendString("DHT INFO: sensor attached.");
}

void FirmataDHT::detachDHTSensor()
{
  if (isAttached())
  {
    free(m_dhtSensor);
    m_dhtSensor = nullptr;
    // m_attachedPin = NOT_A_PIN;
  }
}

boolean FirmataDHT::handlePinMode(byte pin, int mode)
{
  if (mode == PIN_MODE_DHT) {
    if (IS_PIN_INTERRUPT(pin))
    {
      // nothing to do, pins states are managed
      // in "attach/detach" methods
      return true;
    }
  }
  return false;
}

void FirmataDHT::handleCapability(byte pin)
{
  if (IS_PIN_INTERRUPT(pin)) {
    Firmata.write(PIN_MODE_DHT);
    Firmata.write(28); //28 bits used (14 bits for temperature and 14 bits for humidity)
  }
}


/* Handle DHTSENSOR_DATA (0x66) sysex commands
 * See protocol details in "examples/SimpleFirmataDHT/SimpleFirmataDHT.ino"
*/
boolean FirmataDHT::handleSysex(byte command, byte argc, byte *argv)
{
  if (command == DHTSENSOR_DATA)
  {
    byte dhtCommand, pinNum;
    dhtCommand = argv[0];

    if ((dhtCommand == DHTSENSOR_ATTACH_DHT11) || (dhtCommand == DHTSENSOR_ATTACH_DHT22))
    {
      if(argc < 2) return false; // must have at least pin number
      pinNum = argv[1];

      if (Firmata.getPinMode(pinNum) == PIN_MODE_IGNORE)
      {
        return false;
      }

      idDHTLib::DHTType sensorType = (dhtCommand == DHTSENSOR_ATTACH_DHT11) ? idDHTLib::DHTType::DHT11 : idDHTLib::DHTType::DHT22;
      if (argc > 2) {
        bool blockingReads = (bool) argv[2];
        if (argc > 3) {
          unsigned long samplingInterval = 0;
          for (uint8_t i = 3; i < argc; i++) {
            samplingInterval |= long(argv[i] & 0x7F) << ((i - 3) * 7);
          }
          if (argv[argc - 1] & 0b01000000) {
            // Last higher bit is 1, so fill the remaining bits with 1's
            samplingInterval |= -1 << (7 * (argc - 3));
          }
          attachDHTSensor(pinNum, sensorType, blockingReads, samplingInterval);
        } else {
          attachDHTSensor(pinNum, sensorType, blockingReads);
        }
      } else {
        attachDHTSensor(pinNum, sensorType);
      }

      return true;
    }

    if (dhtCommand == DHTSENSOR_DETACH)
    {
      detachDHTSensor();
      return true;
    }

    Firmata.sendString("DHT Error: Invalid command");
  }
  return false;
}

boolean FirmataDHT::isAttached()
{
  return (m_dhtSensor != nullptr);
}

void FirmataDHT::reset()
{
  detachDHTSensor();
}

void FirmataDHT::report()
{
  if (!isAttached()) return;
  Firmata.startSysex();
  Firmata.write(DHTSENSOR_DATA);
  Firmata.sendValueAsTwo7bitBytes(int16_t(round(m_dhtSensor->getCelsius() * 10)));
  Firmata.sendValueAsTwo7bitBytes(uint16_t(round(m_dhtSensor->getHumidity() * 100)));
  Firmata.endSysex();
}

/*==============================================================================
 * LOOP()
 *============================================================================*/
void FirmataDHT::update()
{
  if (!isAttached()) return;
  if (m_dhtSensor->acquiring()) return; // Hasn't finished reading sensor yet

  unsigned long currentMillis = millis();

  if (m_acquiring) // was acquiring, now is finished
  {
    m_acquiring = false;

    int result = m_dhtSensor->getStatus();
    switch (result)
    {
    case IDDHTLIB_OK: 
      m_lastUpdateMillis = currentMillis;
      report();
      return;
      break;
    case IDDHTLIB_ERROR_CHECKSUM: 
      Firmata.sendString("DHT Error: Checksum error");
      break;
    case IDDHTLIB_ERROR_TIMEOUT: 
      Firmata.sendString("DHT Error: Time out error");
      break;
    case IDDHTLIB_ERROR_ACQUIRING: 
      Firmata.sendString("DHT Error: Acquiring");
      break;
    case IDDHTLIB_ERROR_DELTA: 
      Firmata.sendString("DHT Error: Delta time too small");
      break;
    case IDDHTLIB_ERROR_NOTSTARTED: 
      Firmata.sendString("DHT Error: Not started");
      break;
    default: 
      Firmata.sendString("DHT Error: Unknown error");
      break;
    }

    // error, retry in a few ms
    m_lastUpdateMillis = currentMillis + DHT_SENSOR_RETRY_INTERVAL - m_samplingInterval;
  }

  // When millis() wrap the unsigned long limit, this will make it wait longer for the update
  // but most of the time it's useful (for sensor init time and timeouts)
  if (m_dhtSensor && (currentMillis > m_lastUpdateMillis) && ((currentMillis - m_lastUpdateMillis) > m_samplingInterval))
  {
    // Time to get new values from sensor
    // Firmata.sendString("DHTSensor: start acquiring.");
    if (m_blockingReads) {
      // Most of the acquire process is non-blocking, but the start up is very time sensitive
      // Will block for 18ms when sending the start signal to the sensor
      m_dhtSensor->acquire();
    } else {
      // Without the blocking, update() has to be called every < 10ms
      m_dhtSensor->acquireFastLoop();
    }
    m_acquiring = true;
  }
}