TOP=../..

include $(TOP)/configure/CONFIG

# Build a support lib
LIBRARY_IOC += mqttSupport

DBD += mqtt.dbd

mqtt_DBD += base.dbd
mqtt_DBD += asyn.dbd
mqtt_DBD += drvMqtt.dbd
mqtt_DBD += PVAServerRegister.dbd
mqtt_DBD += qsrv.dbd

mqttSupport_SRCS += mqtt_registerRecordDeviceDriver.cpp
mqttSupport_SRCS += drvMqtt.cpp
mqttSupport_SRCS += mqttClient.cpp

mqttSupport_SRCS_DEFAULT += mqttMain.cpp
mqttSupport_SRCS_vxWorks += -nil-

mqttSupport_LIBS += $(EPICS_BASE_IOC_LIBS)
mqttSupport_LIBS += asyn
mqttSupport_LIBS += autoparamDriver
mqttSupport_LIBS += qsrv
mqttSupport_LIBS += $(EPICS_BASE_PVA_CORE_LIBS)

# --------- Paho MQTT Integration ---------

# If installed in non-standard location, set:
# PAHO_CPP_INC and PAHO_CPP_LIB in configure/RELEASE

USR_INCLUDES += -I$(PAHO_CPP_INC)
USR_LDFLAGS += -L$(PAHO_CPP_LIB) -Wl,-rpath,$(PAHO_CPP_LIB)

# nlohmann json.hpp
USR_INCLUDES += -Ijson/json.hpp

mqttSupport_SYS_LIBS += paho-mqttpp3 
mqttSupport_SYS_LIBS += paho-mqtt3as

include $(TOP)/configure/RULES

