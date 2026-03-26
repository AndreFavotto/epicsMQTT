# EPICS Support for the MQTT protocol

This module provides an EPICS driver for the MQTT protocol, allowing EPICS clients to communicate with MQTT brokers and
devices directly from EPICS.

Contributions are welcome - feel free to open issues and pull requests!

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

- Auto-update of EPICS PVS via `I/O Intr` records;
- Support for read/write flat MQTT topics (i.e, topics where the payload is a single value or array);
- Support for read/write JSON payloads with JSON pointer;
- Support for MQTT QoS levels;
- Checks and reject invalid messages (based mostly on type-checking);
- Auto reconnection of broker;
- Planned - short term:
  - Support for MQTT retained messages.
  - Support for MQTT last will messages.
  - Support for MQTT authentication and TLS.

> Note: Virtually all features from the
> [Paho C++ MQTT client](https://eclipse.dev/paho/files/paho.mqtt.python/html/client.html) are available to be
> implemented in this driver, so feel free to open an issue if you need a specific feature.

This module is built on top of [Cosylab autoparamDriver](https://epics.cosylab.com/documentation/autoparamDriver/),
which uses [the standard asyn interfaces](https://epics-modules.github.io/asyn/asynDriver.html#generic-interfaces) for
device support. For now, the supported interfaces are the following:

- `asynInt32`
- `asynFloat64`
- `asynUInt32Digital`
- `asynOctet`
- `asynInt32Array`
- `asynFloat64Array`
  > See [Implementation status](#implementation-status) to check the status of development of the interface you need.

## Building the module

1. Install the dependencies:
   > Tested with the following versions.
   - EPICS Base 7.0.8.1: https://github.com/epics-base/epics-base/releases/tag/R7.0.8.1
   - Asyn 4.45: https://github.com/epics-modules/asyn/releases/tag/R4-45
   - autoparamDriver 2.0.0: https://github.com/Cosylab/autoparamDriver/releases/tag/v2.0.0
   - Paho 1.5.3: Follow
     [these](https://github.com/eclipse-paho/paho.mqtt.cpp?tab=readme-ov-file#build-the-paho-c-and-paho-c-libraries-together)
     instructions.

  > Note: AutoparamDriver explicitly requires EPICS >= 7.0, see
  > [reference](https://github.com/Cosylab/autoparamDriver/blob/main/autoparamDriverSup/src/Makefile#L15). For this reason,
  > this module requires EPICS 7.0 or later to be built and used.

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

   > For now we have two macros for setting paho path because we build the module with separate linking flags -I and -L,
   > but this might change soon.

4. Run `make`. The library should now be ready for [usage](#usage).

## Usage

1. Include the module in your IOC build instructions:
   - Add asyn and mqtt to your `configure/RELEASE` file:

     ```shell
     ## Other definitions ...
     ASYN = /path/to/asyn
     MQTT = /path/to/epicsMqtt
     ## Other definitions ...
     ```

   - Add the mqtt database definition and include the necessary libraries to your `yourApp/src/Makefile`:
     ```shell
     #### Other commands ...
     yourIOC_DBD += mqtt.dbd
     #### Other commands ...
     yourIOC_LIBS += asyn
     yourIOC_LIBS += mqttSupport
     ```

2. In your database file, link the EPICS records and the MQTT topics through the `INP` and `OUT` fields. The syntax is
   as follows:

```shell
  field(INP|OUT, "@asyn(<PORT>) <FORMAT>:<TYPE> <TOPIC> [<FIELD>]")
```

Where:

- `<PORT>` is the name of the asyn port defined in the `asynPortDriver` configuration.
- `<FORMAT>` is the format of the payload: `FLAT` or `JSON`.
- `<TYPE>` is the general type of the expected value [`INT|FLOAT|DIGITAL|STRING|INTARRAY|FLOATARRAY`].
- `<TOPIC>` is the MQTT topic to which the record will be subscribed/published.

> **Note on JSON support:** A configuration file must be loaded by specifying a .json configuration file using mqttJsonDriverConfigure command

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

> Note: Several examples can be found in [mqttExampleApp/Db](mqttExampleApp/Db/example.db) for other record types.

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

## JSON configuration file

JSON records use a configuration file that specifies how to parse or compose
a JSON string. The configuration file should have 1 JSON object in its root.
Each item in this root object refers to an MQTT topic, specified in INP/OUT
field in a EPICS database record.

```json
{
  "readonly_message_root": {
    "fields": {"VAL": ""}
  },
  "readonly_message_dict": {
    "fields": {"VAL": "/1/path/to/key"}
  },
  "write": {
    "fields": {"VAL": "/0"},
    "template": [null, {"metadata": "md_content"}]",
    "writeAddress": "example/write"
  }
}

```

In the JSON snippet above, if all three records received a MQTT message with
string `[3, {"path": {"to": {"key": 2}}}]`, this would be the read result:

- `readonly_message_root`: [3, {"path": {"to": {"key": 2}}}]
- `readonly_message_dict`: 2

For writing,

- `write` would send a message with content `["payload", {"metadata": "md_content"}]`
  if it used a string record and write value was "payload". Since writeAddress
  was specified, it would send the message into topic "example/write". Otherwise,
  the default write topic is identical to the read topic, "write".

## Implementation status

Below are the supported interfaces and their implementation status.


| Message type        | Asyn Parameter Type                    | `FORMAT:TYPE` string to use | Direction    | Status    |
| ------------------- | -------------------------------------- | --------------------------- | ------------ | --------- |
| Integer             | asynInt32                              | `FLAT:INT`                  | Read / Write | Supported |
| Float               | asynFloat64                            | `FLAT:FLOAT`                | Read / Write | Supported |
| Bit masked integers | asynUInt32Digital                      | `FLAT:DIGITAL`              | Read / Write | Supported |
| Strings             | asynOctetRead/asynOctetWrite           | `FLAT:STRING`               | Read / Write | Supported |
| Integer Array       | asynInt32ArrayIn/asynInt32ArrayOut     | `FLAT:INTARRAY`             | Read / Write | Supported |
| Float Array         | asynFloat64ArrayIn/asynFloat64ArrayOut | `FLAT:FLOATARRAY`           | Read / Write | Supported |
| Integer             | asynInt32                              | `JSON:INT`                  | Read / Write | Supported |
| Float               | asynFloat64                            | `JSON:FLOAT`                | Read / Write | Supported |
| Bit masked          | asynUInt32Digital                      | `JSON:DIGITAL`              | Read / Write | Supported |
| String              | asynOctetRead                          | `JSON:STRING`               | Read / Write | Supported |
| Integer Array       | asynInt32ArrayIn/asynInt32ArrayOut     | `JSON:INTARRAY`             | Read / Write | Supported |
| Float Array         | asynFloat64ArrayIn/asynFloat64ArrayOut | `JSON:FLOATARRAY`           | Read / Write | Supported |

## Licensing Terms

Copyright (C) 2026 André Favoto

This program is free software: you can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not,
see <https://www.gnu.org/licenses/>.
