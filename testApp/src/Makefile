# SPDX-FileCopyrightText: 2003 Argonne National Laboratory
#
# SPDX-License-Identifier: EPICS

TOP=../..

include $(TOP)/configure/CONFIG

PROD_IOC = test

DBD += test.dbd

test_DBD += base.dbd
test_DBD += mqtt.dbd
test_DBD += PVAServerRegister.dbd
test_DBD += qsrv.dbd

test_LIBS += asyn
test_LIBS += mqttSupport
test_LIBS += qsrv
test_LIBS += $(EPICS_BASE_PVA_CORE_LIBS)

test_SRCS += test_registerRecordDeviceDriver.cpp

test_SRCS_DEFAULT += testMain.cpp
test_SRCS_vxWorks += -nil-

test_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES

