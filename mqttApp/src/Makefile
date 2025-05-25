TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = mqtt
# mqtt.dbd will be created and installed
DBD += mqtt.dbd

# mqtt.dbd will be made up from these files:
mqtt_DBD += base.dbd
mqtt_DBD += asyn.dbd
mqtt_DBD += drvMqtt.dbd

# Include dbd files from all support applications:
#mqtt_DBD += xxx.dbd

# Add all the support libraries needed by this IOC
#mqtt_LIBS += xxx

# mqtt_registerRecordDeviceDriver.cpp derives from mqtt.dbd
mqtt_SRCS += mqtt_registerRecordDeviceDriver.cpp
mqtt_SRCS += drvMqtt.cpp
mqtt_SRCS += mqttClient.cpp

# Build the main IOC entry point on workstation OSs.
mqtt_SRCS_DEFAULT += mqttMain.cpp
mqtt_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#mqtt_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
mqtt_LIBS += $(EPICS_BASE_IOC_LIBS)
mqtt_LIBS += asyn
mqtt_LIBS += autoparamDriver

# --------- Paho MQTT Integration ---------

# If installed in non-standard location, set:
# PAHO_CPP_INC and PAHO_CPP_LIB in configure/RELEASE

USR_INCLUDES += -I$(PAHO_CPP_INC)
USR_LDFLAGS  += -L$(PAHO_CPP_LIB)

SYS_PROD_LIBS += paho-mqttpp3 paho-mqtt3as

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

