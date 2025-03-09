#!../../bin/linux-x86_64/tempMonIoc

#- SPDX-FileCopyrightText: 2000 Argonne National Laboratory
#-
#- SPDX-License-Identifier: EPICS

#- You may have to change tempMonIoc to something else
#- everywhere it appears in this file

# Load environment paths (this is generated by the EPICS build system)
< envPaths

cd "${TOP}"

# Register all support components
dbLoadDatabase "dbd/tempMonIoc.dbd"
tempMonIoc_registerRecordDeviceDriver pdbbase

# Load the CoreTemp records with the macro P set to MYIOC:
dbLoadRecords("db/CoreTemp.db", "P=MYIOC:")

# Finally, init the IOC
cd "${TOP}/iocBoot/${IOC}"
iocInit
