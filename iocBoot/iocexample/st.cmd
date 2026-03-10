#!../../bin/linux-x86_64/example

#- SPDX-FileCopyrightText: 2003 Argonne National Laboratory
#-
#- SPDX-License-Identifier: EPICS

#- You may have to change example to something else
#- everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/example.dbd"
mqtt_registerRecordDeviceDriver pdbbase

epicsEnvSet("PORT", "test")
epicsEnvSet("BROKER_URL", "mqtt://172.25.123.141:1883")
epicsEnvSet("CLIENT_ID", "mqttEpics")
epicsEnvSet("QOS", "1")
epicsEnvSet("P", "test:")
epicsEnvSet("R", "mqtt:")

mqttDriverConfigure($(PORT), $(BROKER_URL), $(CLIENT_ID), $(QOS))

## Load record instances
dbLoadRecords("db/example.db", "P=$(P), R=$(R), PORT=$(PORT)")
cd "${TOP}/iocBoot/${IOC}"
iocInit
