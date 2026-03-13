# SPDX-FileCopyrightText: 2003 Argonne National Laboratory
#
# SPDX-License-Identifier: EPICS

TOP=../..

include $(TOP)/configure/CONFIG

PROD_IOC = example

DBD += example.dbd

example_DBD += base.dbd
example_DBD += mqtt.dbd
example_DBD += PVAServerRegister.dbd
example_DBD += qsrv.dbd

example_LIBS += asyn
example_LIBS += mqttSupport
example_LIBS += qsrv
example_LIBS += $(EPICS_BASE_PVA_CORE_LIBS)

example_SRCS += example_registerRecordDeviceDriver.cpp

example_SRCS_DEFAULT += exampleMain.cpp
example_SRCS_vxWorks += -nil-

example_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
