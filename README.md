# EPICS Support for the MQTT protocol

This module provides an EPICS driver for the MQTT protocol, allowing EPICS clients to communicate with MQTT brokers and devices directly from EPICS.

**:warning: Not yet ready for production use!**  

This EPICS module is under development - feel free to contribute opening issues and pull requests!

Developing with:
- EPICS Base 7.0.8.1: https://github.com/epics-base/epics-base/releases/tag/R7.0.8.1
- Asyn 4.45: https://github.com/epics-modules/asyn/releases/tag/R4-45
- autoparamDriver 2.0.0: https://github.com/Cosylab/autoparamDriver/releases/tag/v2.0.0
- Paho 1.5.3: https://github.com/eclipse-paho/paho.mqtt.cpp/releases/tag/v1.5.3

---

## Table of Contents
- [EPICS Support for the MQTT protocol](#epics-support-for-the-mqtt-protocol)
  - [Table of Contents](#table-of-contents)
  - [Features](#features)
  - [Install](#install)
  - [Usage](#usage)

## Features
- Auto-update of EPICS PVS via `I/O Intr` records.
- Support for read/write flat MQTT topics (i.e, topics where the payload is a single value or array).
- Support for MQTT QoS levels.
- Planned - short term:
  - Support for parsing fields from one-level JSON TOPIC payloads (read-only - in progress).
  - Support for MQTT retained messages.
  - Support for MQTT authentication and TLS.

Built on top of [autoparamDriver](https://epics.cosylab.com/documentation/autoparamDriver/), the device support available is defined by [the standard asyn interfaces](https://epics-modules.github.io/asyn/asynDriver.html#generic-interfaces). For now, the supported interfaces are the following:
  - `asynInt32`
  - `asynFloat64`
  - `asynUInt32Digital`
  - `asynOctet`
  - `asynInt32Array`
  - `asynFloat64Array`

> Note: Virtually all features from the [Paho C++ MQTT client](https://eclipse.dev/paho/files/paho.mqtt.python/html/client.html) are available to be implemented in this driver, so feel free to open an issue if you need a specific feature.

## Install
TODO: Add instructions for building and installing the module.

## Usage
1. Load the module in your startup script using the following syntax:
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
Below is a table with the supported EPICS types, example of I/O link strings and current status of the implementation:

| Support Type                | Asyn Parameter Type         | FORMAT:TYPE      | Status      |
|-----------------------------|----------------------------|------------------|-------------|
| epicsInt32                  | asynInt32             | `FLAT:INT`       | In progress |
| epicsFloat64                | asynFloat64           | `FLAT:FLOAT`     | In progress |
| epicsUInt32                 | asynUInt32Digital     | `FLAT:DIGITAL`   | In progress |
| Octet                       | asynOctet             | `FLAT:STRING`    | In progress |
| Array&lt;epicsInt32&gt;     | asynInt32Array        | `FLAT:INTARRAY`  | In progress |
| Array&lt;epicsFloat64&gt;   | asynFloat64Array      | `FLAT:FLOATARRAY`| In progress |
