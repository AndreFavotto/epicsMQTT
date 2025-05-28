TOP=../..

include $(TOP)/configure/CONFIG

# Build a support lib
LIBRARY += mqtt

DBD += mqtt.dbd

mqtt_DBD += base.dbd
mqtt_DBD += asyn.dbd
mqtt_DBD += drvMqtt.dbd

# PVA Support - commented out for compatibility with EPICS 3
# mqtt_DBD += PVAServerRegister.dbd
# mqtt_DBD += qsrv.dbd
# mqtt_LIBS += qsrv
# mqtt_LIBS += $(EPICS_BASE_PVA_CORE_LIBS)

mqtt_SRCS += mqtt_registerRecordDeviceDriver.cpp
mqtt_SRCS += drvMqtt.cpp
mqtt_SRCS += mqttClient.cpp

mqtt_SRCS_DEFAULT += mqttMain.cpp
mqtt_SRCS_vxWorks += -nil-

mqtt_LIBS += $(EPICS_BASE_IOC_LIBS)
mqtt_LIBS += asyn
mqtt_LIBS += autoparamDriver

# --------- Paho MQTT Integration ---------

# If installed in non-standard location, set:
# PAHO_CPP_INC and PAHO_CPP_LIB in configure/RELEASE

USR_INCLUDES += -I$(PAHO_CPP_INC)
USR_LDFLAGS += -L$(PAHO_CPP_LIB) -Wl,-rpath,$(PAHO_CPP_LIB)
mqtt_SYS_LIBS += paho-mqttpp3 
mqtt_SYS_LIBS += paho-mqtt3as

include $(TOP)/configure/RULES

