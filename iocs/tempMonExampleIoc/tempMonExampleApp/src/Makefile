# SPDX-FileCopyrightText: 2003 Argonne National Laboratory
#
# SPDX-License-Identifier: EPICS

TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = tempMonIoc
# tempMonIoc.dbd will be created and installed
DBD += tempMonIoc.dbd

# tempMonIoc.dbd will be made up from these files:
tempMonIoc_DBD += base.dbd

# Add the temperature monitor DBD file
tempMonIoc_DBD += tempMon.dbd

# Add qsrv things, so we can access our PVs using PVA
tempMonIoc_DBD += qsrv.dbd
tempMonIoc_DBD += PVAServerRegister.dbd

# Include dbd files from all support applications:
#tempMonIoc_DBD += xxx.dbd

# Add all the support libraries needed by this IOC
#tempMonIoc_LIBS += xxx

# tempMonIoc_registerRecordDeviceDriver.cpp derives from tempMonIoc.dbd
tempMonIoc_SRCS += tempMonIoc_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
tempMonIoc_SRCS_DEFAULT += tempMonIocMain.cpp
tempMonIoc_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#tempMonIoc_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Link in our device support
tempMonIoc_LIBS += tempMon

# Finally link to the EPICS Base libraries
tempMonIoc_LIBS += qsrv
tempMonIoc_LIBS += $(EPICS_BASE_IOC_LIBS)
tempMonIoc_LIBS += $(EPICS_BASE_PVA_CORE_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

