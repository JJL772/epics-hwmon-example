#!../../bin/linux-x86_64/tempMonIoc

#- SPDX-FileCopyrightText: 2000 Argonne National Laboratory
#-
#- SPDX-License-Identifier: EPICS

#- You may have to change tempMonIoc to something else
#- everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/tempMonIoc.dbd"
tempMonIoc_registerRecordDeviceDriver pdbbase

## Load record instances
dbLoadTemplate "db/user.substitutions"
dbLoadRecords "db/tempMonIocVersion.db", "user=jeremy"
dbLoadRecords "db/dbSubExample.db", "user=jeremy"

#- Set this to see messages from mySub
#-var mySubDebug 1

#- Run this to trace the stages of iocInit
#-traceIocInit

cd "${TOP}/iocBoot/${IOC}"
iocInit

## Start any sequence programs
#seq sncExample, "user=jeremy"
