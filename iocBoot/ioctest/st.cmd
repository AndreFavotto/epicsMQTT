#!../../bin/linux-x86_64/test

#- SPDX-FileCopyrightText: 2003 Argonne National Laboratory
#-
#- SPDX-License-Identifier: EPICS

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/test.dbd"
test_registerRecordDeviceDriver pdbbase

epicsEnvSet("PORT", "mqttTest")
epicsEnvSet("BROKER_URL", "mqtt://test.mosquitto.org:1883")
epicsEnvSet("CLIENT_ID", "$(MQTT_TEST_CLIENT_ID=epicsMQTT-test)")
epicsEnvSet("QOS", "1")
epicsEnvSet("P", "mqtt:test:")
epicsEnvSet("R", "")
epicsEnvSet("TOPIC_ROOT", "$(MQTT_TEST_TOPIC_ROOT=epicsMQTT/ci/test)")

mqttDriverConfigure($(PORT), $(BROKER_URL), $(CLIENT_ID), $(QOS))

## Load test records
dbLoadRecords("db/mqttTest.db", "P=$(P),R=$(R),PORT=$(PORT),TOPIC_ROOT=$(TOPIC_ROOT)")

cd "${TOP}/iocBoot/${IOC}"
iocInit
