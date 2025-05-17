#!../../bin/linux-x86_64/mqtt

#- You may have to change mqtt to something else
#- everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/mqtt.dbd"
mqtt_registerRecordDeviceDriver pdbbase

## Load record instances
#dbLoadRecords("db/mqtt.db","user=andrefavotto")

cd "${TOP}/iocBoot/${IOC}"
iocInit

## Start any sequence programs
#seq sncxxx,"user=andrefavotto"
