#!../../bin/linux-x86_64/mqtt

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/mqtt.dbd"
mqtt_registerRecordDeviceDriver pdbbase

epicsEnvSet("PORT", "test")
epicsEnvSet("BROKER_URL", "mqtt://localhost:1883")
epicsEnvSet("CLIENT_ID", "mqttEpics")
epicsEnvSet("QOS", "1")
epicsEnvSet("P", "p:")
epicsEnvSet("R", "r:")

mqttDriverConfigure($(PORT), $(BROKER_URL), $(CLIENT_ID), $(QOS))

## Load record instances
dbLoadRecords("${TOP}/mqttApp/Db/mqtt.db", "P=$(P), R=$(R), PORT=$(PORT)")
cd "${TOP}/iocBoot/${IOC}"
iocInit
