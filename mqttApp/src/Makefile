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

# Include dbd files from all support applications:
#mqtt_DBD += xxx.dbd

# Add all the support libraries needed by this IOC
#mqtt_LIBS += xxx

# mqtt_registerRecordDeviceDriver.cpp derives from mqtt.dbd
mqtt_SRCS += mqtt_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
mqtt_SRCS_DEFAULT += mqttMain.cpp
mqtt_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#mqtt_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
mqtt_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

