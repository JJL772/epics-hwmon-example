# EPICS hwmon example

Sample device support module and test IOC that monitors CPU usage through sysfs on Linux.

Supported hardware:
- Intel x86 processors via the 'coretemp' hwmon driver
- AMD x86 processors (k10 and later) via the 'k10temp' hwmon driver
- Raspberry Pi via the 'cpu\_thermal' hwmon driver

Demonstrates the following EPICS concepts:
- Custom device support for analog input record
- Analog input linear conversion using ESLO & EOFF
- Alarm parameters (HIHI, HIGH, HSV, HHSV)
- Alarm management in device support code
- Simple IOC application that uses the custom device support

## Building

NOTE: This guide assumes you have already followed the environment setup guide on Confluence.

First, generate RELEASE\_SITE:
```sh
$ epics-update -r
```

If you're building this locally on a system that doesn't have `epics-update`, you should create a RELEASE\_SITE that looks like this:
```makefile
# Absolute path to the EPICS modules directory
EPICS_MODULES=/path/to/epics/modules
# This may also be empty if you're not using a version subdirectory
BASE_MODULE_VERSION=R7.0.3.1
# Should point to the top of the EPICS installation area
EPICS_SITE_TOP=/path/to/epics
```

After creating the RELEASE\_SITE, run:
```sh
$ make
```

If you want to build the example IOC along with the device support module, run:
```sh
$ make BUILD_IOCS=YES
```

## Running the IOC

After running `make BUILD_IOCS=YES`, the IOC can be launched with:
```sh
$ cd iocs/tempMonExampleIoc/iocBoot/sioc-temp-mon-test
$ ../../bin/$EPICS_HOST_ARCH/tempMonIoc st.cmd
```

For convenience, I provided a script called `ioc.sh` that runs the IOC for you:
```sh
$ ./ioc.sh
```

## Launching PyDM Screens

### Installing PyDM

PyDM can be installed through conda with the following command:
```sh
$ conda create -n pydm-environment python=3.10 pyqt=5.12.3 pydm -c conda-forge
```
PyQt is pinned to 5.12.3 here per recommendation by the PyDM maintainers. If you don't pin it, Qt designer will not have PyDM widgets in it.
If you don't plan on using Qt designer to create new screens this isn't a big deal.

The new environment can be activated using `conda activate pydm-environment`

### Launching Screens

The screen can be launched by either using `launch-screen.sh`, or with the following command:
```sh
$ pydm -m "P=MYIOC:" ./tempMonApp/op/pydm/cpu_temp.ui
```

The `-m` argument provides a list of macro substitutions. In our case, the macro `P` corresponds
to the PV prefix we put on the record. This matches the value we passed to dbLoadRecords in iocBoot.
