# EPICS Support for the MQTT protocol

This module provides an EPICS driver for the MQTT protocol, allowing EPICS clients to communicate with MQTT brokers and devices directly from EPICS.

**:warning: Not yet ready for production use!**  

This EPICS module is under development - feel free to contribute opening issues and pull requests!

---
## Table of Contents
- [EPICS Support for the MQTT protocol](#epics-support-for-the-mqtt-protocol)
  - [Table of Contents](#table-of-contents)
  - [Features](#features)
  - [Building the module](#building-the-module)
  - [Usage](#usage)
  - [Implementation status](#implementation-status)

---
## Features
- Auto-update of EPICS PVS via `I/O Intr` records.
- Support for read/write flat MQTT topics (i.e, topics where the payload is a single value or array).
- Support for MQTT QoS levels.
- Planned - short term:
  - Several improvements on error handling
  - Support for parsing fields from one-level JSON TOPIC payloads
  - Support for MQTT retained messages.
  - Support for MQTT authentication and TLS.

> Note: Virtually all features from the [Paho C++ MQTT client](https://eclipse.dev/paho/files/paho.mqtt.python/html/client.html) are available to be implemented in this driver, so feel free to open an issue if you need a specific feature.

This module is built on top of [Cosylab autoparamDriver](https://epics.cosylab.com/documentation/autoparamDriver/), which uses [the standard asyn interfaces](https://epics-modules.github.io/asyn/asynDriver.html#generic-interfaces) for device support.
For now, the supported interfaces are the following:
  - `asynInt32`
  - `asynFloat64`
  - `asynUInt32Digital`
  - `asynOctet`
  - `asynInt32Array`
  - `asynFloat64Array`
> Not all of them are fully implemented, so check [Implementation status](#implementation-status) for details.

## Building the module
1. Install the dependencies:
   > The versions below are the ones in use for development, but it is very likely that EPICS Base >= 3.15 should work.
   - EPICS Base 7.0.8.1: https://github.com/epics-base/epics-base/releases/tag/R7.0.8.1
   - Asyn 4.45: https://github.com/epics-modules/asyn/releases/tag/R4-45
   - autoparamDriver 2.0.0: https://github.com/Cosylab/autoparamDriver/releases/tag/v2.0.0
   - Paho 1.5.3: Follow [these](https://github.com/eclipse-paho/paho.mqtt.cpp?tab=readme-ov-file#build-the-paho-c-and-paho-c-libraries-together) instructions.
2. Clone this repository:
  ```shell
    git clone https://github.com/AndreFavotto/epicsMQTT.git
   ```
3. Edit the `configure/RELEASE` file to include your paths to the dependencies:
   ```shell
   EPICS_BASE = /path/to/epics/base
   ASYN = /path/to/asyn
   AUTOPARAM = /path/to/autoparamDriver
   PAHO_CPP_INC = /path/to/paho/cpp/include #by default, should be /usr/local/include
   PAHO_CPP_LIB = /path/to/paho/cpp/lib     #by default, should be /usr/local/lib
   ```
   > For now we have two macros for setting paho path because we build the module with separate linking flags -I and -L, but this will change soon.

4. Run `make`. 
   The library should now be ready for [usage](#usage).
   
## Usage
1. Include the module in your IOC build instructions:
   - Add asyn, autoparam and mqtt to your `configure/RELEASE` file:
        ```shell
        ## Other definitions ...
        ASYN = /path/to/asyn
        AUTOPARAM = /path/to/autoparamDriver
        MQTT = /path/to/epicsMqtt
        ## Other definitions ...
        ```

   - Add .dbd and library definitions to your `yourApp/src/Makefile`:
      ```shell
      #### Other commands ...
      yourIOC_DBD += asyn.dbd
      yourIOC_DBD += mqtt.dbd
      #### Other commands ...
      test_LIBS += asyn
      test_LIBS += autoparamDriver
      test_LIBS += mqtt
      ```
2. In your database file, link the EPICS records and the MQTT topics through the `INP` and `OUT` fields. The syntax is as follows:
  ```shell
    field(INP|OUT, "@asyn(<PORT>) <FORMAT>:<TYPE> <TOPIC> [<FIELD>]")
  ```
  Where:
  - `<PORT>` is the name of the asyn port defined in the `asynPortDriver` configuration.
  - `<FORMAT>`is the format of the payload, either `FLAT` or `JSON`. For now, only `FLAT` is supported.
  - `<TYPE>` is the general type of the expected value [`INT|FLOAT|DIGITAL|STRING|INTARRAY|FLOATARRAY`].  
  - `<TOPIC>` is the MQTT topic to which the record will be subscribed/published.
  - `<FIELD>` is an optional field name to be used when parsing JSON payloads (not yet implemented).
    
  **Important: Due to the pub/sub nature of MQTT, ALL input records are expected to be `I/O Intr`.**

  Example:
  ```console
  record(ai, "$(P)$(R)AnalogIn"){
    field(DESC, "Analog Input Record")
    field(DTYP, "asynInt32")
    field(SCAN, "I/O Intr")
    field(INP, "@asyn($(PORT)) FLAT:INT test/analogtopic")
  }

  record(ai, "$(P)$(R)AnalogOut"){
    field(DESC, "Analog Output Record")
    field(DTYP, "asynInt32")
    field(OUT, "@asyn($(PORT)) FLAT:INT test/analogtopic")
  }
  ```
3. Load the module in your startup script using the following syntax:
  ```cpp
   mqttDriverConfigure(const char *portName, const char *brokerUrl, const char *mqttClientID, const int qos)
  ```
  Example:
  ```shell
    # (... other startup commands ...)
    epicsEnvSet("PORT", "test")
    epicsEnvSet("BROKER_URL", "mqtt://localhost:1883")
    epicsEnvSet("CLIENT_ID", "mqttEpics")
    epicsEnvSet("QOS", "1")
    mqttDriverConfigure($(PORT), $(BROKER_URL), $(CLIENT_ID), $(QOS))
    # (... other startup commands ...)
    dbLoadRecords("your_database.db", "PORT=$(PORT)")
    iocInit()
  ```

## Implementation status

Below is a table with the supported interfaces for the FLAT topics, example of I/O link strings and current status of the implementation. The JSON payload support is planned to come next.

| Message type         | Asyn Parameter Type                    | `FORMAT:TYPE` string to use  | Status      |
|----------------------|--------------------------------------- |------------------------------|-------------|
| Integer              | asynInt32                              | `FLAT:INT`                   | In progress |
| Float                | asynFloat64                            | `FLAT:FLOAT`                 | In progress |
| Bit masked integers  | asynUInt32Digital                      | `FLAT:DIGITAL`               | In progress |
| Strings              | asynOctetRead/asynOctetWrite           | `FLAT:STRING`                | In progress |
| Integer Array        | asynInt32ArrayIn/asynInt32ArrayOut     | `FLAT:INTARRAY`              | In progress |
| Float Array          | asynFloat64ArrayIn/asynFloat64ArrayOut | `FLAT:FLOATARRAY`            | In progress |
